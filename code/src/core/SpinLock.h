#ifndef _RCD_SPINLOCK_H_
#define _RCD_SPINLOCK_H_

#include <atomic>

class SpinLock {
public:
	SpinLock()
		: locked(ATOMIC_FLAG_INIT) {}

	bool tryLock() {
		return locked.test_and_set(std::memory_order_acquire);
	}

	void lock() {
		while(tryLock());
	}

	
	void unlock() {locked.clear(std::memory_order_release);}

private:
	std::atomic_flag locked;
};

#endif
