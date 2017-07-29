#ifndef _RCD_STOCHLINEARSVMPROBLEM_H_
#define _RCD_STOCHLINEARSVMPROBLEM_H_

#include "LinearSVMProblem.h"

class StochLinearSVMNode: public LinearSVMNode {
	typedef LinearSVMNode Super;
public:
	StochLinearSVMNode(int varId)
		: Super(varId) {}

	virtual void init(int phase, const NodeStaticInput &staticInput) override;

	virtual void updateAsSlave(int masterId, const MasterInfo &masterInfo,
			const NodeInput &input, SlaveInfo& slaveInfo,
			Update& slaveUpdate) const override;

protected:
	mutable int stochGradK_;
};

class StochLinearSVMProblem: public LinearSVMProblem {
	typedef LinearSVMProblem Super;
public:
 StochLinearSVMProblem(const Dataset *data, int numFeatures, double C) :
	Super(data, numFeatures, C) {}

	virtual RCDNode<LinearSVMInfoSpec> *createNode(int varId) override {
		return new StochLinearSVMNode(varId);
	}
};

#endif





