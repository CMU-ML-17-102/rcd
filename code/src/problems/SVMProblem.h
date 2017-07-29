#ifndef _RCD_SVMPROBLEM_H_
#define _RCD_SVMPROBLEM_H_

#include <Eigen/SparseCore>
#include "core/Problem.h"
#include "core/RCDNode.h"

struct SVMMasterInfo {
	double kSelf;
};

struct SVMSlaveInfo {
	double masterUpdate;
};

struct SVMStaticInput {
	int numVars;
	double C;
	const double *y;
};

struct SVMNodeInput {
	int numVars;
	const Eigen::VectorXd *alpha;
};

struct SVMInfoSpec {
	typedef SVMMasterInfo MasterInfo;
	typedef SVMSlaveInfo SlaveInfo;
	typedef SVMNodeInput NodeInput;
	typedef SVMStaticInput NodeStaticInput;
};

typedef std::function<double(int, int)> Kernel;

template<class Data = Eigen::SparseVector<double> >
class KernelOnDataset {
public:
	typedef std::vector<Data> Dataset;

	KernelOnDataset(const Dataset &data)
		: data_(data) {}
	
	double operator()(int i, int j) const {
		return kernelFunc(data_[i], data_[j]);
	}

protected:
	virtual double kernelFunc(const Data &x1, const Data &x2) const = 0;

private:
	const std::vector<Data> &data_;
};

template<class Data = Eigen::SparseVector<double> >
class LinearKernel : public KernelOnDataset<Data> {
	typedef typename KernelOnDataset<Data>::Dataset Dataset;
public:
	LinearKernel(const Dataset &data)
		: KernelOnDataset<>(data) {}

protected:
	virtual double kernelFunc(const Data &x1, const Data &x2) const override {
		return x1.dot(x2);
	}
};

class SVMNode: public RCDNode<SVMInfoSpec> {
	typedef RCDNode<SVMInfoSpec> Super;
public:
	SVMNode(int varId, Kernel kernel)
		: Super(varId), kernel_(kernel) {}

	virtual void init(int phase, const NodeStaticInput &staticInput) override;

	virtual MasterInfo getInfoAsMaster(int slaveId,
			const NodeInput& input) const override;
	virtual void updateAsMaster(const MasterInfo &myInfo, int slaveId,
			const SlaveInfo &slaveInfo, Update& masterUpdate) const override;

	virtual void updateAsSlave(int masterId, const MasterInfo &masterInfo,
			const NodeInput &input, SlaveInfo& slaveInfo,
			Update& slaveUpdate) const override;

private:
	int numVars_;
	double C_;
	Kernel kernel_;
	double kSelf_;
	const double *y_;
};

class SVMProblem: public Problem<SVMInfoSpec> {
	typedef Problem<SVMInfoSpec> Super;
	friend class SVMLocalParameterReadClient;
	friend class SVMLocalParameterUpdateClient;
public:
	struct SV {
		double weight;
		int index;
	};

	SVMProblem(int numExamples, Kernel kernel, double C) :
			Super(numExamples, 1), kernel_(kernel), C_(C), b_(0) {
		y_.resize(numExamples);
	}

	double &y(int i) {
		return y_[i];
	}

	double alpha(int i) const {return x_[i][0];}
	double b() const {return b_;}

	virtual ParameterReadClient<SVMNodeInput, SVMStaticInput>
	*createLocalParameterReadClient() override;

	virtual ParameterUpdateClient
		*createLocalParameterUpdateClient() override;

	virtual double computeObjective() const override;
	virtual RCDNode<SVMInfoSpec> *createNode(int varId) override {
		return new SVMNode(varId, kernel_);
	}

	void computeSVsAndIntercept();

private:
	std::vector<double> y_;
	Kernel kernel_;
	double C_;
	double b_;

	std::vector<SV> supportVectors_;
};

#endif
