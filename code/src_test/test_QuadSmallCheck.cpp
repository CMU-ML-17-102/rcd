#include <cassert>
#include "TestUtils.h"

using namespace std;

int main(int argc, char **argv) {
	int debugProcess = -1;
	int numThreads = 1;


	Test test;
	test.numVars = 10;
	test.varDim = 2;
	test.numConstraints = 2;
	test.constraint = CONS_IDENTITY;
	test.layout = LAYOUT_CHAIN;
	test.outputFile = "/dev/null";	
	test.schedule = SCHED_LOCK_FREE;
	test.eps = 1e-6;
	test.maxIterations = 999999999;
	
	Platform::init();
	Platform::setNumLocalThreads(numThreads);

	if(Platform::processId == 0) {
		Platform::printInfo();
	}

	if(Platform::processId == debugProcess)  {Platform::waitForDebugger();}
	OptOutput out;

	ScheduleType schedules[] = {SCHED_SPIN_DOUBLE, SCHED_SPIN_SINGLE, SCHED_LOCK_FREE};

	for(ScheduleType s : schedules) {
		try {
			test.schedule = s;
			out = quickTest(test);

			if(Platform::processId == 0) {
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
		} catch(exception &x) {
			cerr << x.what() << endl;
			Platform::abortMPI();
			return -1;
		} catch(...) {
			cerr << "Caught general exception" << endl;
			Platform::abortMPI();
			return -1;
		}

		double tolerance = 1e-3;
		ASSERT_NEAR(out.objective, 405.0, tolerance);
	}
	
	Platform::finalizeMPI();

	return 0;
}

