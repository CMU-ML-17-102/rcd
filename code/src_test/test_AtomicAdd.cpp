#include <functional>
#include <iostream>
#include <atomic>

#include "core/Platform.h"

using namespace std;

int main(int argc, char **argv) {
	double x = 0;

	#pragma omp parallel for
	for(int i = 0; i < 1000; i++) {
		Platform::atomicAdd(&x, 1);
	}

	assert(x == 1000.0);
	return 0;
}
