#ifndef _RCD_LOCALASYNCSCHEDULER_H_
#define _RCD_LOCALASYNCSCHEDULER_H_

#include <functional>
#include "core/RCDNode.h"
#include "core/RCDScheduler.h"
#include "core/SpinLock.h"
#include "environments/NetConfig.h"
#include "environments/PairSelectionFunc.h"

typedef std::function<PairSelection *(int threadId, int numThreads)> PairSelectionFactory;

template <class InfoSpec, class Lock = SpinLock>
	class LocalAsyncScheduler : public RCDLocalScheduler<InfoSpec> {
	typedef RCDLocalScheduler<InfoSpec> Super;
 public:
	enum LockingLevel {
		DOUBLE,
		SINGLE,
		LOCK_FREE
	};

 public:
	typedef typename Super::MasterInfo MasterInfo;
	typedef typename Super::SlaveInfo SlaveInfo;
	typedef typename Super::Update Update;
	typedef typename Super::NodeInput NodeInput;

	LocalAsyncScheduler(PairSelectionFactory &factory) {
		construct(factory);
	}

	LocalAsyncScheduler(int numVars) {
		PairSelectionFactory factory =
		[numVars, this] (int threadId, int numThreads) -> PairSelection* {
			return new RandomSinglePair(numVars, threadId);};
		construct(factory);
	}

	LocalAsyncScheduler(const NetConfig *config) {
		PairSelectionFactory factory =
		[config, this] (int threadId, int numThreads) -> PairSelection* {
			return new RandomSinglePair(config, threadId);};
		construct(factory);
	}

	~LocalAsyncScheduler() {
		for(int i = 0; i < numThreads; i++) {
			delete selectors[i];
		}

		delete selectors;
	}

	void setSyncPeriod(int p) {this->syncPeriod = p;}
	void setLockingLevel(LockingLevel level) {this->locking = level;}

 protected:
	virtual OptOutput doSolve() OVERRIDE;

 private:
	void construct(PairSelectionFactory &factory);

	int numVars;
	int numThreads;
	int syncPeriod;
	bool outputObjectiveTrace;
	LockingLevel locking;
	PairSelection **selectors;
};

#include "LocalAsyncScheduler_Impl.h"

#endif
