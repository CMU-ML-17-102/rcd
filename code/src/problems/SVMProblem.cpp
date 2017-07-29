#include <cmath>
#include "SVMProblem.h"
#include "core/SpinLock.h"

#define EPSILON 1e-6
inline bool equal(double x, double y) {
	double eps = EPSILON * (x + y + EPSILON);
	if(eps < 0) {eps = -eps;}
	double diff = x - y;
	return diff < eps && diff > -eps;
}

class SVMLocalParameterReadClient : public ParameterReadClient<SVMNodeInput, SVMStaticInput> {
public:
	SVMLocalParameterReadClient(SVMProblem *problem)
		: problem_(problem) {}

	virtual void getNodeInput(int varId, SVMNodeInput &nodeInput) OVERRIDE {
		nodeInput.alpha = &(problem_->x_[0]);
		nodeInput.numVars = problem_->numVars_;
	}

	virtual SVMStaticInput getNodeStaticInput(int varId) OVERRIDE {
		SVMStaticInput staticInput;
		staticInput.C = problem_->C_;
		staticInput.numVars = problem_->numVars_;
		staticInput.y = &(problem_->y_[0]);
		return staticInput;
	}

private:
	SVMProblem *problem_;
};

class SVMLocalParameterUpdateClient : public LocalParameterUpdateClient<SVMInfoSpec> {
	typedef LocalParameterUpdateClient<SVMInfoSpec> Super;
public:
	SVMLocalParameterUpdateClient(SVMProblem *problem)
		: Super(problem), problem_(problem) {
		locks_ = new SpinLock[problem->numVars_];
	}

	~SVMLocalParameterUpdateClient() {
		if(locks_) {delete locks_;}
	}

	virtual void update(int varId1, int varId2, const Eigen::VectorXd& increment1,
				const Eigen::VectorXd& increment2, bool async) OVERRIDE {
		//In SVM we need to need to scale updates to maintain box constraint
		int id1 = varId1, id2 = varId2;
		double delta1 = increment1[0];
		double delta2 = increment2[0];

		if (varId2 < varId1) {
			id1 = varId2;
			id2 = varId1;
			delta1 = increment2[0];
			delta2 = increment1[0];
		}

		locks_[id1].lock();
		locks_[id2].lock();

		double a1 = problem_->x_[id1][0];
		double a2 = problem_->x_[id2][0];
		double a1new = a1 + delta1;
		double a2new = a2 + delta2;

		double scale = 1.0;
		double C = problem_->C_;

		if(a1new < 0) {
			double s = a1/(a1 - a1new);
			if(s < scale) {scale = s;}
		} else if(a1new > C) {
			double s = (C - a1)/(a1new - a1);
			if(s < scale) {scale = s;}
		}

		if(a2new < 0) {
			double s = a2/(a2 - a2new);
			if(s < scale) {scale = s;}
		} else if(a2new > C) {
			double s = (C - a2)/(a2new - a2);
			if(s < scale) {scale = s;}
		}

		assert(scale >= 0.0);

		problem_->x_[id1][0] = a1 + scale * delta1;
		problem_->x_[id2][0] = a2 + scale * delta2;

		locks_[id2].unlock();
		locks_[id1].unlock();
	}

private:
	SVMProblem *problem_;
	SpinLock *locks_;
};

//=============================================================================
// SVMNode
//=============================================================================

void SVMNode::init(int phase, const SVMNode::NodeStaticInput &staticInput) {
	if(phase != 0) {return;}
	kSelf_ = kernel_(varId_, varId_);
	assert(kSelf_ > 0);
	C_ = staticInput.C;
	numVars_ = staticInput.numVars;
	y_ = staticInput.y;
}

SVMNode::MasterInfo SVMNode::getInfoAsMaster(int slaveId,
	const SVMNode::NodeInput& input) const {
	SVMMasterInfo info;
	info.kSelf = kSelf_;
	return info;
}

void SVMNode::updateAsMaster(const SVMNode::MasterInfo &myInfo, int slaveId,
	const SVMNode::SlaveInfo &slaveInfo, SVMNode::Update& masterUpdate) const {
	masterUpdate.resize(1);
	masterUpdate[0] = slaveInfo.masterUpdate;
}

void SVMNode::updateAsSlave(int masterId, const SVMNode::MasterInfo &masterInfo,
	const SVMNode::NodeInput &input, SVMNode::SlaveInfo& slaveInfo,
	SVMNode::Update& slaveUpdate) const {
	int i = masterId; int j = varId_;
	double alphai = input.alpha[i][0];
	double alphaj = input.alpha[j][0];

	double R = 0.0;
	double Kii = masterInfo.kSelf;
	double Kjj = kSelf_;
	double Kij = kernel_(masterId, varId_);
	double yi = y_[i];
	double yj = y_[j];
	double yij = yi * yj;

	double S = alphai * yi + alphaj * yj;

	for(int k = 0; k < numVars_; ++k) {
		if(k != i && k != j && input.alpha[k][0] > 0.0) {
			R += input.alpha[k][0] * y_[k] * yi * (kernel_(k,i) - kernel_(k,j));
		}
	}

	double K_denom = (-Kii - Kjj + 2*Kij);
	assert(Kii > 0.0);
	assert(Kjj > 0.0);

	double ai = alphai;
	if(K_denom != 0.0) { // Can fail for repeated training points
		ai = (yij - 1 + R + S*yi*(Kij-Kjj)) / K_denom;
		assert(!std::isnan(ai));
	}

	double lower, upper;

	if(yij > 0) {
		lower = S * yj - C_;
		upper = S * yj;
	} else {
		lower = -S * yj;
		upper = C_ - S * yj;
	}

	if(lower < 0.0) {lower = 0.0;}
	if(upper > C_) {upper = C_;}
	assert(lower <= upper);

	if(ai > upper) {ai = upper;}
	else if(ai < lower) {ai = lower;}
	double aj = (S - ai * yi) * yj;
	ASSERT(aj >= 0, aj);
	ASSERT(aj <= C_, aj);

	slaveInfo.masterUpdate = ai - alphai;
	slaveUpdate.resize(1);
	slaveUpdate[0] = aj - alphaj;
}

//=============================================================================
// SVMProblem
//=============================================================================

ParameterReadClient<SVMNodeInput, SVMStaticInput> *SVMProblem::createLocalParameterReadClient() {
	return new SVMLocalParameterReadClient(this);
}

ParameterUpdateClient *SVMProblem::createLocalParameterUpdateClient() {
	return new SVMLocalParameterUpdateClient(this);
}

double SVMProblem::computeObjective() const {
	double sum = 0.0;

	for(int i = 0; i < numVars_; ++i) {
		for(int j = 0; j < numVars_; ++j) {
			sum += kernel_(i,j) * y_[i] * y_[j]
				* x_[i][0] * x_[j][0];
		}
	}

	sum /= 2.0;

	for(int i = 0; i < numVars_; ++i) {
		sum -= x_[i][0];
	}

	return sum;
}

void SVMProblem::computeSVsAndIntercept() {
	supportVectors_.clear();

	for(int i = 0; i < numVars_; ++i) {
		if(!equal(x_[i][0], 0.0)) {
			SV v;
			v.index = i;
			v.weight = x_[i][0];
			supportVectors_.push_back(v);
		}
	}

	int n = supportVectors_.size();

	double minScore = INFINITY;
	double maxScore = -INFINITY;

	for(int i = 0; i < n; i++) {
		double score = 0.0;

		for(int j = 0; j < n; j++) {
			score += supportVectors_[j].weight
					* kernel_(supportVectors_[i].index, supportVectors_[j].index)
					* y_[supportVectors_[j].index];
		}

		if(score < minScore) {minScore = score;}
		if(score > maxScore) {maxScore = score;}
	}

	b_ = (-minScore - maxScore) / 2.0;
}
