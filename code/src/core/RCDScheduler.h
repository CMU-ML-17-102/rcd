#ifndef _RCD_RCDSCHEDULER_H

#define _RCD_RCDSCHEDULER_H

#include <functional>
#include <limits>
#include <unordered_map>
#include <string>
#include "core/RCDNode.h"
#include "core/Platform.h"
#include "core/Problem.h"

typedef std::function<void (int iteration, int time, double objective)> IterationListener;

/**
 * Reports information on the optimization procedure (e.g. number of iterations executed)
 */
struct OptOutput {
	int numIterations;
	int timems;
	double objective;

	std::unordered_map<std::string, int> propInt;
	std::unordered_map<std::string, double> propDouble;
};

class Sender {
 public:
	virtual ~Sender() {}
	virtual void send(const char *data, int length) = 0;
};

class Reciever {
 public:
	virtual ~Reciever() {}
	virtual void recieve(char *data, int length) = 0;
};

enum StoppingCondition {
	CHANGE = 0, //Stop when change in objective goes below epsilon or number of iterations excceeds maxIterations
	OBJECTIVE = 1, //Stop when objective goes below epsilon or number of iterations excceeds maxIterations
	ITERATIONS = 2 //Stop after exceeding maxIterations
};

/**
 * RCDScheduler is responsible for executing the algorithm, taking care of
 * pair selection, synchronoization, communication ... etc. The scheduler manages 
 * a set of RCDSymNodes representing local variables (i.e. variables assignned to the local machines).
 *
 * The template parameters specify the type of information exchanged between two RCDNodes in the 
 * pair update process. A pair update proceeds as follows: One nodes acts as a master and the other acts as a slave, and the following sequence occurs: [1] get info from master [2] slave uses master info to generate slave info and update [3] master uses master info and slave info to update. 
 * Extracting the info and using it for variable update is the responsibility of the RCDNode.
 */
template <class InfoSpec>
class RCDScheduler {
 public:	
	typedef RCDNode<InfoSpec> Node;
	typedef typename InfoSpec::MasterInfo MasterInfo;
	typedef typename InfoSpec::SlaveInfo SlaveInfo;
	typedef typename RCDNode<InfoSpec>::Update Update;
	typedef typename InfoSpec::NodeInput NodeInput;
	typedef typename InfoSpec::NodeStaticInput NodeStaticInput;
    typedef ParameterReadClient<NodeInput, NodeStaticInput> ParamClient;

 RCDScheduler()
	 : maxIterations_(100000), eps_(1e-5)
		, minObj_(-std::numeric_limits<double>::infinity())
		{}

	virtual ~RCDScheduler() {}

	void setMaxIterations(int maxIterations) {maxIterations_ = maxIterations;}
	void setObjTolerance(double eps) {eps_ = eps;}
	void setMinObjective(double minObj) {minObj_ = minObj;}

	void setListenerIteration(IterationListener listener) {iterationListener_ = listener;}
	void setParameterReadClient(ParameterReadClient<NodeInput, NodeStaticInput> *readClient) {
		this->readClient_ = readClient;
	}
	void setParameterUpdateClient(ParameterUpdateClient *updateClient) {
		this->updateClient_ = updateClient;
	}

	void addNode(Node *node) {
		nodes_.push_back(node);
	}

	const Node *getNode(int i) const {return nodes_[i];}

	void deleteNodes() {
		for(Node *node : nodes_) {delete node;}
	}
	
	virtual OptOutput solve();

 protected:
	int getElapsedTime() const;
	virtual OptOutput doSolve() = 0;

 	IterationListener iterationListener_;
 	std::vector<Node *> nodes_;
	int maxIterations_;
	double eps_;
	double minObj_;
	StoppingCondition stopping_; //TODO: make a predicate
	Platform::Time startTime_;
	int start_;
	int end_;

	ParameterReadClient<NodeInput, NodeStaticInput> *readClient_;
	ParameterUpdateClient *updateClient_;
};

template <class InfoSpec>
class RCDLocalScheduler : public RCDScheduler<InfoSpec> {
	typedef RCDScheduler<InfoSpec> Super;
public:
	virtual void readProblem(Problem<InfoSpec> *problem);

protected:
	Problem<InfoSpec> *problem_;
};

template <class InfoSpec>
OptOutput RCDScheduler<InfoSpec>::solve() {
	startTime_ = Platform::getCurrentTime();
	LOG("Scheduler started");
	int numVars = nodes_.size();
	int numInitPhases = nodes_[0]->getNumInitPhases();

	for(int phase = 0; phase < numInitPhases; ++phase) {
#pragma omp parallel for schedule(dynamic)
		for(int i = 0; i < numVars; ++i) {
			updateClient_->init(i);
			const NodeStaticInput &input = readClient_->getNodeStaticInput(i);
			nodes_[i]->init(phase, input);
		}

		LOG("Initialization phase " << phase << "/" << numInitPhases <<  " compeleted");
	}

	OptOutput output = doSolve();
	output.timems = getElapsedTime();
	return output;
}

template<class InfoSpec>
int RCDScheduler<InfoSpec>::getElapsedTime() const {
	auto currentTime = Platform::getCurrentTime();
	return Platform::getDurationms(startTime_, currentTime);
}

template<class InfoSpec>
void RCDLocalScheduler<InfoSpec>::readProblem(Problem<InfoSpec> *problem) {
	this->problem_ = problem;
	this->start_ = 0;
	this->end_ = problem->numVars();
	this->nodes_.clear();
	this->nodes_.reserve(problem->numVars());

	for(int i = this->start_; i < this->end_; i++) {
		this->nodes_.push_back(problem->createNode(i));
	}

	this->readClient_ = problem->createLocalParameterReadClient();
	this->updateClient_ = problem->createLocalParameterUpdateClient();
}

#endif
