#ifndef _RCD_SVMTOOLS_H
#define _RCD_SVMTOOLS_H

#include <Eigen/Dense>
#include <Eigen/SparseCore>

class SVMUtils {
 public:
	static void readCsvFile(const char *fileName, std::vector<Eigen::VectorXd> &data, std::vector<double> &labels);
	static void readSvmFile(const char *fileName, std::vector<Eigen::SparseVector<double> > &data, std::vector<double> &labels, int &numFeatures);	
};

#endif
