#ifndef _RCD_PROBLEM_H_
#define _RCD_PROBLEM_H_

#include <functional>
#include <Eigen/Dense>
#include "core/ParameterClient.h"

typedef std::function<double (const Eigen::VectorXd &)> SingleVarFunction;
typedef	std::function<void (const Eigen::VectorXd &, Eigen::VectorXd &)> SingleVarGradient;

typedef std::function<double (const Eigen::VectorXd *)> MultiVarFunction;

//Given the current values of all blocks and an int specifying a block id,
//output the gradient w.r.t the given block.
typedef	std::function<void (const Eigen::VectorXd *, int, Eigen::VectorXd &)> MultiVarGradient;

//Given block id, return corresponding Lipschitz constant for gradient
typedef std::function<double (int)> BlockLipschitz;

// Represents a a bloackbox oracle for 
// a smooth function that is applied to a single block.
// A seperable objective is represented by a set of RCDSepFunction objects.
struct RCDSepFunction {
	RCDSepFunction() {}
	RCDSepFunction(SingleVarFunction f, SingleVarGradient grad, double L)
		: f(f), grad(grad), L(L) {}

	SingleVarFunction f;
	SingleVarGradient grad;
	double L;	//Lipschitz constant for gradient
};

// Represents a blackbox oracle for 
// a smooth function that is applied to multiple blocks.
struct RCDSmoothFunction {
	RCDSmoothFunction() {}
    RCDSmoothFunction(
					  MultiVarFunction f, MultiVarGradient grad,
					  BlockLipschitz L)
        : f(f), grad(grad), L(L) {}

	MultiVarFunction f;
	MultiVarGradient grad;	
	BlockLipschitz L;
};

template<class InfoSpec> class Problem;

template<class InfoSpec>
class LocalParameterUpdateClient : public ParameterUpdateClient {
public:
	LocalParameterUpdateClient(Problem<InfoSpec> *problem)
		: problem_(problem) {}

	virtual void update(int varId1, int varId2, const Eigen::VectorXd& increment1,
			const Eigen::VectorXd& increment2, bool async) OVERRIDE;

	virtual void init(int varId) OVERRIDE;

private:
	Problem<InfoSpec> *problem_;
};

template<class NodeInfo> class RCDNode;

template<class NodeInfoSpec>
class Problem {
	friend class LocalParameterUpdateClient<NodeInfoSpec>;
public:
	Problem(int numVars, int varDim) :
			numVars_(numVars), varDim_(varDim) {
		x_.resize(numVars);
	}

	~Problem() {}

	int numVars() const {return numVars_;}
	virtual ParameterReadClient<typename NodeInfoSpec::NodeInput,
			typename NodeInfoSpec::NodeStaticInput>
	*createLocalParameterReadClient() = 0;
	virtual ParameterUpdateClient *createLocalParameterUpdateClient() {
		return new LocalParameterUpdateClient<NodeInfoSpec>(this);
	}

	virtual RCDNode<NodeInfoSpec> *createNode(int varId) = 0;
	virtual double computeObjective() const = 0;

protected:
	std::vector<Eigen::VectorXd> x_;
	int numVars_;
	int varDim_;
};

template<class InfoSpec>
void LocalParameterUpdateClient<InfoSpec>::update(int varId1, int varId2, const Eigen::VectorXd& increment1,
			const Eigen::VectorXd& increment2, bool async) {
	if(varId1 >= 0) {
		Platform::updateVector(problem_->x_[varId1], increment1, async);
	}

	if(varId2 >= 0) {
		Platform::updateVector(problem_->x_[varId2], increment2, async);
	}
}

template<class InfoSpec>
void LocalParameterUpdateClient<InfoSpec>::init(int varId) {
	problem_->x_[varId] = Eigen::VectorXd(problem_->varDim_);
}

template<class NodeInfoSpec>
class LinearConstraintsProblem: public Problem<NodeInfoSpec> {
	typedef Problem<NodeInfoSpec> Super;
public:
	LinearConstraintsProblem(int numVars, int varDim) :
			Super(numVars, varDim) {
		A_.resize(numVars);
	}

	virtual Eigen::VectorXd computeConstraintFunction() const {
		Eigen::VectorXd out = A_[0] * this->x_[0];
		for (int i = 1; i < this->numVars_; ++i) {
			out += A_[i] * this->x_[i];
		}
		return out;
	}

	Eigen::MatrixXd &A(int i) {
		return A_[i];
	}

protected:
	std::vector<Eigen::MatrixXd> A_;
};

#endif
