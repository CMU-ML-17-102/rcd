#ifndef _RCD_LOCALASYNCSCHEDULERIMPL_H_
#define _RCD_LOCALASYNCSCHEDULERIMPL_H_

#include <atomic>
#include <algorithm>
#include <cmath>

#include "core/Platform.h"
#include "environments/FlagSet.h"
#include "environments/LocalAsyncScheduler.h"
#include "environments/PairSelectionFunc.h"

template<class InfoSpec, class Lock>
	void LocalAsyncScheduler<InfoSpec, Lock>::construct(PairSelectionFactory &factory) {
	outputObjectiveTrace = true;
	locking = SINGLE;
	numThreads = Platform::getNumLocalThreads();
	selectors = new PairSelection*[numThreads];

	#pragma omp parallel
	{
		int tid = Platform::getThreadId();
		selectors[tid] = factory(tid, numThreads); 
		assert(selectors[tid]->getBufferSize() == 1);
	}
}

template<class InfoSpec, class Lock>
	OptOutput LocalAsyncScheduler<InfoSpec, Lock>::doSolve() {
	OptOutput output;
	int numVars = this->nodes_.size();
	Lock locks[numVars];
	int version[numVars];

	std::fill(version, version + numVars, 0);
	
	if(syncPeriod > this->maxIterations_) {syncPeriod = this->maxIterations_;}
	double prevObj = std::numeric_limits<double>::infinity();

	bool converged = false;
	int numUpdates[numThreads]; std::fill(numUpdates, numUpdates + numThreads, 0);
	int numCollisions[numThreads]; std::fill(numCollisions, numCollisions + numThreads, 0);
	int numAscents[numThreads]; std::fill(numAscents, numAscents + numThreads, 0);

	FlagSet flags(numVars);

	while(!converged) {
		LOG("Iteration BLOCK");
		bool convTest = false;

		#pragma omp parallel shared(convTest)
		{
		int tid = Platform::getThreadId();

		while(!convTest) {		  	
			int i, j, n;
			//double objOld = 0.0;
			//double objNew = 0.0;
			
			selectors[tid]->getPairs(&i, &j, n);
			assert(i != j);

			int id1 = (i < j) ?i :j;
			int id2 = (i < j) ?j :i;

			//To avoid deadlocks, always lock the node with the smallest id first
			if(locking == DOUBLE) {locks[id1].lock(); locks[id2].lock(); }

			if(locking == SINGLE) {locks[i].lock();}
			int versionOld = version[i];

			NodeInput input_i; this->readClient_->getNodeInput(i, input_i);
			MasterInfo info_i = this->nodes_[i]->getInfoAsMaster(j, input_i);
			//Platform::sleepCurrentThread(50);
			if(locking == SINGLE) {locks[i].unlock();}

			if(locking == SINGLE) {locks[j].lock();}

			SlaveInfo info_j;
			Update update_j;		
			NodeInput input_j; this->readClient_->getNodeInput(j, input_j);
			this->nodes_[j]->updateAsSlave(i, info_i, input_j, info_j, update_j);
			//Platform::sleepCurrentThread(50); <-- Important

			if(locking == SINGLE) {
				this->updateClient_->update(j, -1, update_j, update_j, false);
			}

			if(locking == SINGLE) {locks[j].unlock();}

			if(locking == SINGLE) {locks[i].lock();}
			int versionNew = version[i]++;
			Update update_i;
			this->nodes_[i]->updateAsMaster(info_i, j, info_j, update_i);

			if(locking == SINGLE) {
				this->updateClient_->update(i, -1, update_i, update_i, false);
			} else {
				this->updateClient_->update(i, j, update_i, update_j, locking == LOCK_FREE);
			}

			if(locking == SINGLE) {locks[i].unlock();}

			if(locking == DOUBLE) {locks[id2].unlock(); locks[id1].unlock(); }

			++numUpdates[tid];
			flags.set(i); flags.set(j);
			if(tid == 0 && numUpdates[tid] % 1000 == 0) {
				int totalUpdates = std::accumulate(numUpdates, numUpdates + numThreads, 0);
				LOG(totalUpdates);

				if(this->maxIterations_ > 0 && totalUpdates > this->maxIterations_) {convTest = true;}
			}

			if(flags.getNumActive() == numVars) {convTest = true;}

			//Make sense only when locking level is single
			if(versionNew != versionOld) {
				++numCollisions[tid];
			}
			//if(objNew > objOld) {
			//	++numAscents[tid];
			//}
		}
		LOG(tid << "done");		
	
		}		

		flags.reset();

		//Compute objective
		int totalUpdates = std::accumulate(numUpdates, numUpdates + numThreads, 0);
		LOG(totalUpdates << " Conv test");
		double sum = 0.0;

		if(this->problem_) {
			sum = this->problem_->computeObjective();
		}

		if((this->eps_ > 0.0 && fabs(prevObj - sum) < this->eps_)
		   || sum < this->minObj_
		   || (this->maxIterations_ > 0 && totalUpdates > this->maxIterations_)) {
			converged = true;
			LOG("Converged");
		} else {prevObj = sum;}

		if(outputObjectiveTrace && this->iterationListener_) {
			this->iterationListener_(totalUpdates, this->getElapsedTime(), sum);
		}
	}

	LOG("Finished");
	
	//Compute objective
	double finalObjective = 0.0;
	if(this->problem_) {finalObjective = this->problem_->computeObjective();}
	output.objective = finalObjective;
	output.numIterations = std::accumulate(numUpdates, numUpdates + numThreads, 0);
	output.propInt["collisions"] = std::accumulate(numCollisions, numCollisions + numThreads, 0);
	output.propInt["ascents"] = std::accumulate(numAscents, numAscents + numThreads, 0);
	return output;
}

#endif
