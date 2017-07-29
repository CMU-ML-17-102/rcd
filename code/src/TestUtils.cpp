#include <cmath>
#include <vector>
#include <cassert>
#include <mutex>
#include <Eigen/Dense>
#include "core/Platform.h"
#include "core/Problem.h"
#include "core/SpinLock.h"
#include "environments/LocalAsyncScheduler.h"
#include "environments/NetConfig.h"
#include "environments/PairSelectionFunc.h"
#include "problems/SepSmoothObjectiveProblem.h"
#include "TestUtils.h"
#include "RCDIterationLogger.h"

using namespace std;
using namespace Eigen;

SepSmoothObjectiveProblem createQuadFunctions(int numVars, int varDim, int start, int end) {
	double c = 1.0;

	SepSmoothObjectiveProblem output(numVars, varDim);
	
	//Compute c such that the objective at 0 <= 1000
	for(int i = start; i < end; i++) {
		int k = (i % 10);
		for(int ii = 0; ii < varDim; ii++) { c += k*k; }
	}

	c = c / 1000.0;
	if(c < 1.0) {c = 1.0;}

	for(int i = start; i < end; i++) {
		int k = (i % 10);

		SingleVarFunction f = [k, varDim, c] (const Eigen::VectorXd &x) -> double {
			double out = 0.0;
			for(int ii = 0; ii < varDim; ii++) {
				out += (x[ii] - k) * (x[ii] - k) / c;
			}
			
			return out;
		};

		SingleVarGradient grad = [k, varDim, c] (const Eigen::VectorXd &x, Eigen::VectorXd &out) {
			out.resize(varDim);

			for(int ii = 0; ii < varDim; ii++) {
				out[ii] = 2.0 * (x[ii]-k) / c;
			}
		};

		output.function(i) = RCDSepFunction(f, grad, 2.0 / c);
	}

	return output;
}

OptOutput quickTest(const Test &test) {
	typedef RCDLocalScheduler<SepSmoothObjInfoSpec> Scheduler;
	typedef LocalAsyncScheduler<SepSmoothObjInfoSpec, SpinLock> SpinAsyncScheduler;
	typedef LocalAsyncScheduler<SepSmoothObjInfoSpec> LockFreeScheduler;
 
	int start, end, chunk;
	Platform::getProcessRange(test.numVars, start, end, chunk);

	//Create communication graph
	NetConfig config(test.numVars);

	switch(test.layout) {
	case LAYOUT_CHAIN:
		config = NetConfig::createChain(test.numVars);
		break;

	case LAYOUT_RING:
		config = NetConfig::createRing(test.numVars);
		break;

	case LAYOUT_CLIQUE:
		config = NetConfig::createClique(test.numVars);
		break;

	case LAYOUT_TREE:
		config = NetConfig::createTree(test.numVars);
		break;

	case LAYOUT_STAR:
		config = NetConfig::createStar(test.numVars);
		break;

	case LAYOUT_SUPERTREE:
		config = NetConfig::createSuperTree(test.numVars);
		break;
	}

	Scheduler *rcd;
	OfflinePairs pairs;

	RandomKPairs kpairs(&config, Platform::getNumLocalThreads(), 0);
	CoveringPairs cover(&config, 0);
	PairSelectionFactory factory = [config] (int threadId, int numThreads) -> PairSelection * {
		OfflinePairs *p = new OfflinePairs();
		RandomSinglePair base(&config, threadId);
		p->init(&base, 2000000); //TODO: Remove hardcoded number
		return p;
	};

	int numRecords = 0;

	switch(test.schedule) {
	case SCHED_SPIN_DOUBLE:
		rcd = new SpinAsyncScheduler(factory);
		if(test.syncPeriod > 0) {static_cast<SpinAsyncScheduler *>(rcd)->setSyncPeriod(test.syncPeriod);}
		static_cast<SpinAsyncScheduler *>(rcd)->setLockingLevel(LockFreeScheduler::DOUBLE);
		break;

	case SCHED_SPIN_SINGLE:
		rcd = new SpinAsyncScheduler(factory);
		if(test.syncPeriod > 0) {static_cast<SpinAsyncScheduler *>(rcd)->setSyncPeriod(test.syncPeriod);}
			break;

	case SCHED_LOCK_FREE:		
		rcd = new LockFreeScheduler(factory);
		static_cast<LockFreeScheduler *>(rcd)->setLockingLevel(LockFreeScheduler::LOCK_FREE);
	if(test.syncPeriod > 0) {static_cast<LockFreeScheduler *>(rcd)->setSyncPeriod(test.syncPeriod);}
		break;
	}

	rcd->setMaxIterations(test.maxIterations);
	//rcd->setStoppingCondition(test.stopping);
	if(test.eps > 0) { rcd->setObjTolerance(test.eps); }

	//Specify functions
	SepSmoothObjectiveProblem problem = 
		createQuadFunctions(test.numVars, test.varDim, start, end); 

	//Specify constraints
	vector<MatrixXd> A;

	for(int i = start; i < end; i++) {
		MatrixXd Ai;

		switch(test.constraint) {
		case CONS_IDENTITY:
			assert(test.numConstraints == test.varDim);
			Ai = MatrixXd::Identity(test.numConstraints, test.varDim);
			break;

		case CONS_RANDOM_NONNEG:
			Ai = MatrixXd::Random(test.numConstraints, test.varDim);
			Ai = Ai.array().abs();
			break;
		}

		if(test.condition > 0.0) {
			JacobiSVD<MatrixXd> svd(Ai, ComputeThinU | ComputeThinV);
			MatrixXd V = svd.matrixV();
			MatrixXd U = svd.matrixU();
			VectorXd S = svd.singularValues();	

			int j;
			for(j = S.size() - 1; j >= 0 && S(j) == 0; j--);
			
			double r = pow(test.condition, 1.0/j);		
			double s = r;
			j--;

			for(; j >= 0; j--) {
				U.col(j) *= s;
				s *= r;
			}

			Ai = U * V.transpose();
		}
			
		problem.A(i) = Ai;
	}

	RCDIterationLogger logger(test.outputFile, 1);
	rcd->setListenerIteration(logger.listenerHandle);

	rcd->readProblem(&problem);

	//Solve using RCD
	OptOutput out = rcd->solve();

	//Check constraints
	VectorXd d = problem.computeConstraintFunction();
	out.propDouble["Linear Constraint Violation"] = d.maxCoeff();
   	rcd->deleteNodes();
	delete rcd;

	return out;
}


