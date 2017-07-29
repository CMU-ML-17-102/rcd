#ifndef _RCD_FLAGSET_H
#define _RCD_FLAGSET_H

#include <memory>
#include "core/SpinLock.h"

class FlagSet {
 public:
	FlagSet(int numFlags);
	void set(int flag);
	void reset();
	int getNumActive() const;

 private:
	int numFlags;
	int numThreads;
	std::unique_ptr<SpinLock[]> flags;
	std::unique_ptr<int[]> numActive;
};

#endif
