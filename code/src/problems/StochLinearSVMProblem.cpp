#include <cmath>
#include "StochLinearSVMProblem.h"
#include "core/SpinLock.h"

#define EPSILON 1e-6
inline bool equal(double x, double y) {
	double eps = EPSILON * (x + y + EPSILON);
	if(eps < 0) {eps = -eps;}
	double diff = x - y;
	return diff < eps && diff > -eps;
}

//=============================================================================
// StochLinearSVMNode
//=============================================================================

void StochLinearSVMNode::init(int phase, 
							  const StochLinearSVMNode::NodeStaticInput
							  &staticInput) {
	Super::init(phase, staticInput);
	stochGradK_ = 0;
}

void StochLinearSVMNode::updateAsSlave(int masterId, const StochLinearSVMNode::MasterInfo &masterInfo,
	const StochLinearSVMNode::NodeInput &input, StochLinearSVMNode::SlaveInfo& slaveInfo,
	StochLinearSVMNode::Update& slaveUpdate) const {
	int i = masterId; int j = varId_;
	double alphai = input.alpha[i][0];
	double alphaj = input.alpha[j][0];

	double R = 0.0;
	double Kii = masterInfo.kSelf;
	double Kjj = kSelf_;

	const auto &data_i = (*data_)[i];
	const auto &data_j = (*data_)[j];

	double Kij = data_i.dot(data_j);
	double yi = y_[i];
	double yj = y_[j];
	double yij = yi * yj;

	double S = alphai * yi + alphaj * yj;

	if(stochGradK_ == i) {stochGradK_ = (stochGradK_ + 1) % numVars_;}
	if(stochGradK_ == j) {stochGradK_ = (stochGradK_ + 1) % numVars_;}
	if(stochGradK_ == i) {stochGradK_ = (stochGradK_ + 1) % numVars_;}
	int k = stochGradK_;
	stochGradK_ = (stochGradK_ + 1) % numVars_;

	Data ddiff = data_i - data_j;
	const auto &data_k = (*data_)[k];

	if(k != i && k != j) {
		R = 0.5 * numVars_ * input.alpha[k][0] * y_[k] * yi * data_k.dot(ddiff);
	}

	double K_denom = (-Kii - Kjj + 2*Kij);
	assert(Kii >= 0.0);
	assert(Kjj >= 0.0);

	double ai = alphai;
	if(K_denom != 0.0) { // Can fail for repeated training points
		ai = (yij - 1 + R) / K_denom;
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

