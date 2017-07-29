#include <iostream>
#include <Eigen/Dense>
#include <cstdlib>
#include <cstring>
#include "core/Platform.h"
#include "environments/NetConfig.h"
#include "RCDIterationLogger.h"
#include "TestUtils.h"

using namespace std;
using namespace Eigen;

int main(int argc, char **argv) {
	if(argc == 1) {
		cerr << "Usage: " << argv[0] << "<num_vars> <dimension> <num_constraints> <layout> <output_file> [options]";
	}

	int debugProcess = -1;
	int numThreads = 1;

	//Read command line options
	Test test;
	int idx = 1;
	test.numVars = atoi(argv[idx++]);
	test.varDim = atoi(argv[idx++]);
	test.numConstraints = atoi(argv[idx++]);
	test.layout = (Layout) atoi(argv[idx++]);
	test.outputFile = argv[idx++];

	while(idx < argc) {
		const char *param = argv[idx++];

		if(strcmp(param, "cond") == 0) {
			if(idx == argc) {
				cerr << "Command line option argument missing" << endl;
				return -1;
			} else {
				test.condition = atof(argv[idx++]);
			}
		} else if(strcmp(param, "schedule") == 0) {
			if(idx == argc) {
				cerr << "Command line option argument missing" << endl;
				return -1;
			} else {
				test.schedule = (ScheduleType) atoi(argv[idx++]);
			}
		} else if(strcmp(param, "threads") == 0) {
			if(idx == argc) {
				cerr << "Command line option argument missing" << endl;
				return -1;
			} else {
				numThreads = atoi(argv[idx++]);
			}
		} else if(strcmp(param, "iter") == 0) {
			if(idx == argc) {
				cerr << "Command line option argument missing" << endl;
				return -1;
			} else {
				test.maxIterations = atoi(argv[idx++]);
			}
		} else if(strcmp(param, "constraint") == 0) {
			if(idx == argc) {
				cerr << "Command line option argument missing" << endl;
				return -1;
			} else {
				test.constraint = (ConstraintType) atoi(argv[idx++]);
			}
		} else if(strcmp(param, "eps") == 0) {
			if(idx == argc) {
				cerr << "Command line option argument missing" << endl;
				return -1;
			} else {
				test.stopping = CHANGE;
				test.eps = atof(argv[idx++]);
			}
		} else if(strcmp(param, "target") == 0) {
			if(idx == argc) {
				cerr << "Command line option argument missing" << endl;
				return -1;
			} else {
				test.stopping = OBJECTIVE;
				test.eps = atof(argv[idx++]);
			}
		} else if(strcmp(param, "sync") == 0) {
			if(idx == argc) {
				cerr << "Command line option argument missing" << endl;
				return -1;
			} else {
				test.syncPeriod = atoi(argv[idx++]);
			}
		} else {
			cerr << "Invalid option: " << param << endl;
			return -1;
		}
	}
	
	Platform::init();
	Platform::setNumLocalThreads(numThreads);

	if(Platform::processId == 0) {
		Platform::printInfo();
	}

	if(Platform::processId == debugProcess)  {Platform::waitForDebugger();}

	try {
		OptOutput out = quickTest(test);

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
	} catch(...) {
		cerr << "Caught general exception" << endl;
		Platform::abortMPI();
	}
	
	Platform::finalizeMPI();

	return 0;
}

