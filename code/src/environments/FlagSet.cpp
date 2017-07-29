#include <algorithm>
#include "core/Platform.h"
#include "environments/FlagSet.h"

FlagSet::FlagSet(int numFlags) 
	: numFlags(numFlags), numThreads(Platform::getNumLocalThreads()) {
	numActive.reset(new int[numThreads]);
	reset();
}

void FlagSet::reset() {
	std::fill(numActive.get(), numActive.get() + numThreads, 0);
	flags.reset(new SpinLock[numFlags]);
}

void FlagSet::set(int flag) {
	if(!flags[flag].tryLock()) {
		++numActive[Platform::getThreadId()];
	}
}

int FlagSet::getNumActive() const {
	return std::accumulate(numActive.get(), numActive.get() + numThreads, 0);
} 


