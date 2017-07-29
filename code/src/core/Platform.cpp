#include <iostream>
#include <cstdlib>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stacktrace.h>
#include <Eigen/Core>
#include "core/Platform.h"

#ifdef USE_MPI
#include <mpi.h>
#endif

#ifdef USE_OPENMP
#include <omp.h>
#endif

using namespace std;

int Platform::processId;
int Platform::numProcesses;

void handleSIGSEV(int sig) {
	print_stacktrace();
	exit(1);
}

void Platform::init() {
#ifdef USE_MPI
	int provided;

	//Initialize MPI
	MPI_Init_thread(0, 0, MPI_THREAD_MULTIPLE, &provided);

	if(provided < MPI_THREAD_MULTIPLE) {
		MPI_Abort(MPI_COMM_WORLD, 1);
	}
    
	//Get MPI info
	MPI_Comm_rank(MPI_COMM_WORLD, &processId);
	MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);
#else
	processId = 0;
	numProcesses = 1;
#endif

	Eigen::initParallel();
	Eigen::setNbThreads(1);

	//Reigster segmentation fault handler
	signal(SIGSEGV, handleSIGSEV);
	signal(SIGABRT, handleSIGSEV);
}

void Platform::finalizeMPI() {
#ifdef USE_MPI
  MPI_Finalize();
#endif
}

void Platform::abortMPI() {
#ifdef USE_MPI
	MPI_Abort(MPI_COMM_WORLD, 1);
#endif
}

int Platform::getNumLocalThreads() {
#ifdef USE_OPENMP
	return omp_get_max_threads();
#else
	return 1;
#endif
}

void Platform::setNumLocalThreads(int n) {
#ifdef USE_OPENMP
	return omp_set_num_threads(n);
#endif
}

int Platform::getThreadId() {
#ifdef USE_OPENMP
	return omp_get_thread_num();
#else
	return 0;
#endif
}

void Platform::sleepCurrentThread(int microseconds) {
	usleep(microseconds);
}

void Platform::waitForDebugger() {
  bool dbg = false; //Set to true to indicate attachment
  cerr << "Process " << getpid() << " is ready for attach" << endl;

  while(!dbg) {sleep(1);}
}


void Platform::updateVector(Eigen::VectorXd &v, const Eigen::VectorXd &increment,
		bool atomicComponentUpdates) {
	if(atomicComponentUpdates) {
		double *raw = v.data();
		int n = v.size();

		for(int i = 0; i < n; i++) {
			Platform::atomicAdd(raw + i, increment(i));
		}
	} else {
		//double *raw = v.data();
		//int n = v.size();

		//for(int i = 0; i < n; i++) {
		//	raw[i] += increment(i);
		//}
		v += increment;
	}
}

void Platform::updateVector(Eigen::VectorXd &v,
			const Eigen::SparseVector<double> &increment, bool atomicComponentUpdates) {
	if(atomicComponentUpdates) {
		double *raw = v.data();
		Eigen::SparseVector<double>::InnerIterator iterator(increment);
		for(; iterator; ++iterator) {
			int idx = iterator.index();
			Platform::atomicAdd(raw + idx, iterator.value());
		}
	} else {
		v += increment;
	}
}

void Platform::copyVector(const Eigen::VectorXd &src,
		Eigen::VectorXd &dst, bool atomicReads) {
	if(atomicReads) {
		int n = src.size();
		dst = Eigen::VectorXd(src);

		for(int i = 0; i < n; ++i) {
			dst(i) = Platform::atomicRead(src.data() + i);
		}
	} else {
		dst = src;
	}
}


