#ifndef _RCD_SEPSMOOTHOBJECTIVEPROBLEM_H
#define _RCD_SEPSMOOTHOBJECTIVEPROBLEM_H

#include <memory>
#include "core/Problem.h"

struct SmoothObjMasterInfo {
	std::unique_ptr<Eigen::VectorXd> scaledGrad; //grad/L
	std::unique_ptr<Eigen::VectorXd> Ad; //A * scaledGrad
	const Eigen::MatrixXd *AAT ;
};

struct SmoothObjSlaveInfo {
	std::unique_ptr<Eigen::VectorXd> lagrange;
	const Eigen::MatrixXd *pairMatrix; //pinv(Ai Ai' / Li + Aj Aj' / Lj)
};

typedef const Eigen::VectorXd *SepSmoothNodeInput;

struct SepSmoothObjInfoSpec {
	typedef SmoothObjMasterInfo MasterInfo;
	typedef SmoothObjSlaveInfo SlaveInfo;
	typedef SepSmoothNodeInput NodeInput;
	typedef DefaultStaticInput NodeStaticInput;
};

class SepSmoothObjectiveProblem 
: public LinearConstraintsProblem<SepSmoothObjInfoSpec> {
	friend class SepSmoothParameterReadClient;
	typedef LinearConstraintsProblem<SepSmoothObjInfoSpec> Super;
 public:
 SepSmoothObjectiveProblem(int numVars, int varDim)
	 : Super(numVars, varDim) {
		func_.resize(numVars);
	}

	virtual double computeObjective() const OVERRIDE;
	virtual ParameterReadClient<SepSmoothNodeInput> *createLocalParameterReadClient() OVERRIDE;
	virtual RCDNode<SepSmoothObjInfoSpec> *createNode(int varId) OVERRIDE;
	RCDSepFunction& function(int varId) { return func_[varId]; }

 private:
	std::vector<RCDSepFunction> func_;
};

/*
class SmoothObjectiveProblem :public LinearConstraintsProblem<> {
	typedef LinearConstraintsProblem<> Super;
 public:
    SmoothObjectiveProblem(const RCDSmoothFunction &function,
						int numVars, int varDim) 
		: Super(numVars, varDim), function(function) {}

 private:	
	RCDSmoothFunction function;	
};
*/

#endif
