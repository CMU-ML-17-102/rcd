#include "core/Platform.h"
#include "problems/SepSmoothObjectiveProblem.h"
#include "problems/SepSmoothObjNode.h"

double SepSmoothObjectiveProblem::computeObjective() const {
	double objective = 0.0;

	for(int i = 0; i < numVars_; i++) {
		objective += func_[i].f(x_[i]);
	}

	return objective;
}

class SepSmoothParameterReadClient : public ParameterReadClient<SepSmoothNodeInput> {
public:
	SepSmoothParameterReadClient(SepSmoothObjectiveProblem *problem)
	: problem(problem) {}

	void getNodeInput(int varId, SepSmoothNodeInput &nodeInput) OVERRIDE {
		nodeInput = &(problem->x_[varId]);
	}

	DefaultStaticInput getNodeStaticInput(int varId) OVERRIDE {
		DefaultStaticInput input;
		input.A = &(problem->A_[varId]);
		input.L = problem->func_[varId].L;
		return input;
	}

private:	
	SepSmoothObjectiveProblem *problem;
};

ParameterReadClient<SepSmoothNodeInput> *SepSmoothObjectiveProblem::createLocalParameterReadClient() {
	return new SepSmoothParameterReadClient(this);
}

RCDNode<SepSmoothObjInfoSpec> *SepSmoothObjectiveProblem::createNode(int varId) {
	return new SepSmoothObjNode(varId, numVars_, varDim_, func_[varId].grad);
}
