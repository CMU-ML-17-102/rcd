#include "core/Platform.h"
#include "core/RCDScheduler.h"
#include "environments/LocalAsyncScheduler.h"
#include "environments/NetConfig.h"
#include "problems/SVMProblem.h"
#include "RCDIterationLogger.h"

int main(int argc, char **argv) {
	int numExamples = 10;

	Platform::init();
	Platform::setNumLocalThreads(1);
	NetConfig config = NetConfig::createClique(numExamples);
	typedef  LocalAsyncScheduler<SVMInfoSpec> Scheduler;
	//RandomKPairs kpairs(&config, Platform::getNumLocalThreads(), 0);
	Scheduler *scheduler = new Scheduler(&config);
	scheduler->setLockingLevel(Scheduler::DOUBLE);

	Kernel kernel = [] (int i, int j) -> double {
			return (1.0*i*j) + 1.0;};

	SVMProblem problem(numExamples, kernel, 1e100);

	int boundary = 6;

	for(int i = 0; i < numExamples; ++i) {
		problem.y(i) = (i < boundary) ?-1.0 :1.0;
	}

	scheduler->readProblem(&problem);

	scheduler->setObjTolerance(0.0);
	scheduler->setMaxIterations(50000);
	RCDIterationLogger logger("/dev/null", 1);
	scheduler->setListenerIteration(logger.listenerHandle);
	scheduler->solve();
	scheduler->deleteNodes();
	delete scheduler;

	double w0 = 0.0;
	double w1 = 0.0;

	for(int i = 0; i < numExamples; i++) {
		if(problem.alpha(i) != 0) {
			LOG("alpha(" << i << ")= " <<  problem.alpha(i));
		}

		w0 += problem.alpha(i) * (1.0 * i) * problem.y(i);
		w1 += problem.alpha(i) * problem.y(i);
	}

	problem.computeSVsAndIntercept();
	LOG(w0 << " " << w1 << " " << problem.b());

	// Check that the only support vectors are boundary points
	for(int i = 0; i < numExamples; ++i) {
		if(i == boundary - 1)  {
			assert(problem.alpha(i) != 0);
		} else if(i == boundary)  {
			assert(problem.alpha(i) != 0);
		} else {
			assert(problem.alpha(i) == 0.0);
		}
	}

	// Check the correct classification of boundary points
	ASSERT_NEAR(w0 * (boundary-1) + w1 + problem.b(), -1, 1e-3);
	ASSERT_NEAR(w0 * boundary + w1 + problem.b(), 1, 1e-3);
	
	return 0;
}
