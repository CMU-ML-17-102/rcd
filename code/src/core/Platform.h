#ifndef _RCD_PLATFORM_H_
#define _RCD_PLATFORM_H_

#include <chrono>
#include <cmath>
#include <iostream>
#include <atomic>
#include <Eigen/Core>
#include <Eigen/SparseCore>

/**
 * Encapsulates platform functions related to parallelism, timing, 
 * atomic operations ... etc.
 */
class Platform {
public:
	typedef std::chrono::time_point<std::chrono::system_clock> Time;

	static int processId;
	static int numProcesses;

	static void init();
	static void finalizeMPI();
	static void abortMPI();
	static int getNumLocalThreads();
	static void setNumLocalThreads(int n);	
	static int getThreadId();
	static void sleepCurrentThread(int microseconds);

	/** 
	Divides a range [0..(n-1)] among processes and returns the local range
	[start..(end-1)] of the current process

	@param[in]	n Specifies input range [0..(n-1)]
	@param[out]	start Beginning of local range [start..(end-1)]
	@param[end]	end of local range [start--(end-1)]
	*/
	static void getProcessRange(int n, int &start, int &end) {
		int chunk;
		getProcessRange(n, start, end, chunk);
	}

	static void getProcessRange(int n, int &start, int &end, int &chunk) {
		chunk = (int) ceil(n * 1.0 / Platform::numProcesses);
		start = chunk * Platform::processId;
		end = chunk * (Platform::processId + 1);
		if(end > n) {end = n;}	
	}

	static void waitForDebugger();

	static void printInfo(std::ostream &out = std::cerr) {
		out << "Process " << processId << " of " << numProcesses << std::endl;
		out << "Number of local threads: " << getNumLocalThreads() << std::endl;	
	}

	static Time getCurrentTime() {
		return std::chrono::system_clock::now();
	}

	static int getDurationms(const Time &start, const Time &end) {
		return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	}

	static double atomicRead(const volatile double *var) {
		//Since compare_and_swap does not support double, 
		//we treat the memory location as "long long" (64-bit integer)
		static_assert(sizeof(long long) == sizeof(double),
					  "Must use an integral type of the same size as double");
		double out;
		volatile long long *pVar = reinterpret_cast<volatile long long *>(const_cast<volatile double*>(var));
		long long *pOut = reinterpret_cast<long long *>(&out);
		*pOut = __sync_val_compare_and_swap(pVar, 0, 0);
		return out;
	}

	static void atomicAdd(volatile double *var, double increment) {
		bool succeed = false;

		//Since compare_and_swap does not support double,
		//we treat the memory location as "long long" (64-bit integer)
		static_assert(sizeof(long long) == sizeof(double),
					  "Must use an integral type of the same size as double");
		do {
			volatile long long *pVar = reinterpret_cast<volatile long long *>(var);
			double val = *var;
			double newVal = val + increment;
			long long *pVal = reinterpret_cast<long long *>(& val);
			long long *pNew = reinterpret_cast<long long *>(& newVal);

			succeed = __sync_bool_compare_and_swap(pVar, *pVal, *pNew);
		}while(!succeed);
	}

	static void updateVector(Eigen::VectorXd &v,
			const Eigen::VectorXd &increment, bool atomicComponentUpdates);


	static void updateVector(Eigen::VectorXd &v,
			const Eigen::SparseVector<double> &increment, bool atomicComponentUpdates);

	static void copyVector(const Eigen::VectorXd &src, Eigen::VectorXd &dst,
			bool atomicRead);
};

#endif

