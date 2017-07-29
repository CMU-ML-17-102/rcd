#ifndef _RCD_RCDITERATIONLOGGER_H_
#define _RCD_RCDITERATIONLOGGER_H_

#include <fstream>
#include "core/RCDScheduler.h"
#include "core/Platform.h"

class RCDIterationLogger {
public:
 RCDIterationLogger(const char *outputFile, int stdErrInterval)
	 : output(outputFile), stdErrInterval(stdErrInterval), lastStdErr(-1) {
		listenerHandle = [this] (int it, int time, double obj) {log(it, time, obj);};
	}
		
	void log(int iteration, int time, double objective) {
		if(Platform::processId != 0) {return;}

		if(stdErrInterval > 0 && (lastStdErr == -1 || (iteration - lastStdErr) >= stdErrInterval)) {
			std::cerr.precision(10);
			std::cerr << "Iteration " << iteration << " done. Current objective is " << std::scientific << objective << std::endl;
			lastStdErr = iteration;
		}

		output.precision(10);
		output << iteration << "," << time << "," << std::scientific << objective << std::endl;
	}
	
	IterationListener listenerHandle;

private:
	std::ofstream output;
	bool outToStdErr;
	int stdErrInterval;
	int lastStdErr;
};

#endif
