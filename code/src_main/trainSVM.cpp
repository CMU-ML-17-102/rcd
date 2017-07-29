#include "CommandLineArgsReader.h"
#include "environments/LocalAsyncScheduler.h"
#include "problems/SVMUtils.h"
#include "problems/SVMProblem.h"
#include "RCDIterationLogger.h"

using namespace std;
using namespace Eigen;

int main(int argc, const char **argv) {
	//Read Parameters
	CommandLineArgsReader argsReader;
	argsReader.read(argc, argv);
	string fileName = argsReader.getParam("--train_file", "");
	int numThreads = atoi(argsReader.getParam("--num_threads", "1").c_str());
	string minObjStr = argsReader.getParam("--min_obj", "ninf");
	double minObj = -std::numeric_limits<double>::infinity();
	if(minObjStr[0] != 'n') {minObj = atof(minObjStr.c_str());}

	vector<SparseVector<double> > data; vector<double> y; int numFeatures;
	SVMUtils::readSvmFile(fileName.c_str(), data, y, numFeatures);

	LinearKernel<> kernel(data);
	
	int numExamples = data.size();

	Platform::init();
	Platform::setNumLocalThreads(numThreads);
	cerr << "Using " << Platform::getNumLocalThreads() << " threads" << endl;

	typedef  LocalAsyncScheduler<SVMInfoSpec> Scheduler;

	Scheduler *scheduler = new Scheduler(numExamples);
	scheduler->setLockingLevel(Scheduler::DOUBLE);

	SVMProblem problem(numExamples, kernel, 1.0);

	for(int i = 0; i < numExamples; ++i) {
		problem.y(i) = y[i];
	}

	scheduler->readProblem(&problem);
	LOG("Processed data");

	scheduler->setObjTolerance(0.0);
	scheduler->setMaxIterations(1e6);
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
