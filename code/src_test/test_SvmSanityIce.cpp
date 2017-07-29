#ifdef USE_ICE

#include <thread>
#include <chrono>
#include <Ice/Ice.h>

#include "problems/SVMProblem.h"
#include "problems/SVMCodec.h"
#include "environments/ParameterClientIceWrapper.h"
#include "environments/LocalAsyncScheduler.h"
#include "RCDIterationLogger.h"

Ice::CommunicatorPtr ic;
int numExamples = 10;

void validate(SVMProblem &problem, int boundary) {
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
}

void server() {
	int boundary = 6;

	Kernel kernel = [] (int i, int j) -> double {
				return (1.0*i*j) + 1.0;};

	SVMProblem problem(numExamples, kernel, 1e100);

	for(int i = 0; i < numExamples; ++i) {
		problem.y(i) = (i < boundary) ?-1.0 :1.0;
	}

	ParameterIceServer<SVMInfoSpec, SVMCodec> server(&problem);
	server.startServer(ic, "default -p 10000", "default -p 15000", "ReadServant", "UpdateServant");

	ic->waitForShutdown();

	validate(problem, boundary);
};

void client() {
	ParameterReadIceClient<SVMInfoSpec::NodeInput, SVMInfoSpec::NodeStaticInput, SVMCodec> readClient;
	readClient.connect(ic, "ReadServant: default -p 10000");

	ParameterUpdateIceClient updateClient;
	updateClient.connect(ic, "UpdateServant: default -p 15000");

	typedef  LocalAsyncScheduler<SVMInfoSpec> Scheduler;
	NetConfig config = NetConfig::createClique(numExamples);
	Scheduler *scheduler = new Scheduler(&config);
	scheduler->setLockingLevel(Scheduler::DOUBLE);

	Kernel kernel = [] (int i, int j) -> double {
				return (1.0*i*j) + 1.0;};

	scheduler->setObjTolerance(0.0);
	scheduler->setMaxIterations(50000);
	RCDIterationLogger logger("/dev/null", 1);
	scheduler->setListenerIteration(logger.listenerHandle);

	scheduler->setParameterReadClient(&readClient);
	scheduler->setParameterUpdateClient(&updateClient);
	scheduler->setMaxIterations(2000);
	scheduler->setObjTolerance(0.0);

	for(int i = 0; i < numExamples; i++) {
		scheduler->addNode(new SVMNode(i, kernel));
	}

	scheduler->solve();
	scheduler->deleteNodes();
	delete scheduler;
};

int main(int argc, char **argv) {
	Platform::init();
	Platform::setNumLocalThreads(1);
	ic = Ice::initialize(argc, argv);

	std::thread serverThread(server);
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	std::thread clientThread1(client);
	std::thread clientThread2(client);

	clientThread1.join();
	clientThread2.join();
	ic->shutdown();
	serverThread.join();
}

#else
int main() {}
#endif
