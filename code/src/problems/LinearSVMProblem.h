#ifndef _RCD_LINEARSVMPROBLEM_H_
#define _RCD_LINEARSVMPROBLEM_H_

#include "SVMProblem.h"

typedef Eigen::SparseVector<double> Data;
typedef std::vector<Data> Dataset;

struct LinearSVMNodeInput {
	int numVars;
	const Eigen::VectorXd *alpha;
	const Eigen::VectorXd *w;
	const Dataset *dataset;
};

struct LinearSVMStaticInput {
	int numVars;
	double C;
	const double *y;
	const Dataset *data;	
};

struct LinearSVMInfoSpec {
	typedef SVMMasterInfo MasterInfo;
	typedef SVMSlaveInfo SlaveInfo;
	typedef LinearSVMNodeInput NodeInput;
	typedef LinearSVMStaticInput NodeStaticInput;
};

class LinearSVMNode: public RCDNode<LinearSVMInfoSpec> {
	typedef RCDNode<LinearSVMInfoSpec> Super;
public:
	LinearSVMNode(int varId)
		: Super(varId) {}

	virtual void init(int phase, const NodeStaticInput &staticInput) override;

	virtual MasterInfo getInfoAsMaster(int slaveId,
			const NodeInput& input) const override;
	virtual void updateAsMaster(const MasterInfo &myInfo, int slaveId,
			const SlaveInfo &slaveInfo, Update& masterUpdate) const override;

	virtual void updateAsSlave(int masterId, const MasterInfo &masterInfo,
			const NodeInput &input, SlaveInfo& slaveInfo,
			Update& slaveUpdate) const override;

protected:
	int numVars_;
	double C_;
	double kSelf_;
	const double *y_;
	const Dataset *data_;
};

class LinearSVMProblem: public Problem<LinearSVMInfoSpec> {
	typedef Problem<LinearSVMInfoSpec> Super;
	friend class LinearSVMLocalParameterReadClient;
	friend class LinearSVMLocalParameterUpdateClient;
public:
	struct SV {
		double weight;
		int index;
	};

 LinearSVMProblem(const Dataset *data, int numFeatures, double C) :
			Super(data->size(), 1), data_(data), C_(C), b_(0) {
				w_ = Eigen::VectorXd::Zero(numFeatures);
				y_.resize(data->size());
	}

	double &y(int i) {
		return y_[i];
	}

	double alpha(int i) const {return x_[i][0];}
	double b() const {return b_;}

	virtual ParameterReadClient<LinearSVMNodeInput, LinearSVMStaticInput>
	*createLocalParameterReadClient() override;

	virtual ParameterUpdateClient
		*createLocalParameterUpdateClient() override;

	virtual double computeObjective() const override;
	virtual RCDNode<LinearSVMInfoSpec> *createNode(int varId) override {
		return new LinearSVMNode(varId);
	}

	void computeSVsAndIntercept();

protected:
	const Dataset *data_;
	Eigen::VectorXd w_;
	std::vector<double> y_;
	double C_;
	double b_;

	std::vector<SV> supportVectors_;
};

#endif





