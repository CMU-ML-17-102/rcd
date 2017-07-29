#include <memory>

#include "CommandLineArgsReader.h"
#include "environments/LocalAsyncScheduler.h"
#include "problems/SVMUtils.h"
#include "problems/StochLinearSVMProblem.h"
#include "RCDIterationLogger.h"

using namespace std;
using namespace Eigen;

int main(int argc, const char **argv) {
	//Read Parameters
	CommandLineArgsReader argsReader;
	argsReader.read(argc, argv);
	string fileName = argsReader.getParam("--train_file", "");
	int numThreads = atoi(argsReader.getParam("--num_threads", "1").c_str());
	int maxIterations = atoi(argsReader.getParam("--iterations", "1000000").c_str());
	bool stochastic = static_cast<bool>(atoi(argsReader.getParam("--stoch", "0").c_str()));
	
	string minObjStr = argsReader.getParam("--min_obj", "ninf");
	double minObj = -std::numeric_limits<double>::infinity();
	if(minObjStr[0] != 'n') {minObj = atof(minObjStr.c_str());}

	Dataset data; vector<double> y; int numFeatures;
	SVMUtils::readSvmFile(fileName.c_str(), data, y, numFeatures);

	int numExamples = data.size();

	Platform::init();
	Platform::setNumLocalThreads(numThreads);
	cerr << "Using " << Platform::getNumLocalThreads() << " threads" << endl;

	typedef  LocalAsyncScheduler<LinearSVMInfoSpec> Scheduler;

	Scheduler *scheduler = new Scheduler(numExamples);
	scheduler->setLockingLevel(Scheduler::DOUBLE);

	std::unique_ptr<LinearSVMProblem> problem;
	if(stochastic) {
		problem.reset(new StochLinearSVMProblem(&data, numFeatures, 1.0));
	} else {
		problem.reset(new LinearSVMProblem(&data, numFeatures, 1.0));
	}

	for(int i = 0; i < numExamples; ++i) {
		problem->y(i) = y[i];
	}

	scheduler->readProblem(problem.get());
	LOG("Processed data");

	scheduler->setObjTolerance(0.0);
	scheduler->setMaxIterations(maxIterations);
	scheduler->setMinObjective(minObj);
	RCDIterationLogger logger("/dev/null", 1);
	scheduler->setListenerIteration(logger.listenerHandle);
	OptOutput out = scheduler->solve();
	scheduler->deleteNodes();
	delete scheduler;

	cout << "Objective = " << out.objective << endl;
	cout << "Iterations = " << out.numIterations << endl;
	cout << "Time = " << (double) out.timems << endl;

	for(const auto &x : out.propInt) {
		cout << x.first << " = " << x.second << endl;
	}

	for(const auto &x : out.propDouble) {
		cout << x.first << " = " << x.second << endl;
	}	
}
