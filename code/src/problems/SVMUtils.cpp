#include "SVMUtils.h"
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>

using namespace std;
using namespace Eigen;

void SVMUtils::readSvmFile(const char *fileName, std::vector<SparseVector<double> > &data, std::vector<double> &labels, int &numFeatures) {
	ifstream in(fileName);
	//TODO: Check file

	string line, token;
	int linenum = 0;
	numFeatures = -1;

	vector<vector<int> > indices;
	vector<vector<double> > vals;

	while(getline(in, line)) {
		++linenum;
		double label;
		vector<string> tokens;
		istringstream linestream(line);

		while(getline(linestream, token, ' ')) {			
			tokens.push_back(token);
		}

		label = atof(tokens[0].c_str());
		labels.push_back(label);

		indices.push_back(vector<int>());
		vals.push_back(vector<double>());

		vector<int> &idcs = indices.back();
		vector<double> &vls = vals.back();
	
		for(size_t i = 1; i < tokens.size(); ++i) {
			size_t colon = tokens[i].find(':');
			ASSERT(colon > 0, "line=" << linenum << " i=" << i << " token=" << tokens[i]);

			char *str = const_cast<char *>(tokens[i].c_str());
			str[colon] = 0;
			int idx = atoi(str);
			double val = atof(str + colon + 1);

			idcs.push_back(idx-1);
			vls.push_back(val);
			if(idx > numFeatures) {numFeatures = idx;}   		
		}

		if(linenum % 1000 == 0) {LOG("Read " << linenum << " examples");}
	}

	LOG("# of features = " << numFeatures);

	for(size_t i = 0; i < indices.size(); ++i) {
		data.push_back(SparseVector<double>(numFeatures));		
		SparseVector<double> &newVector = data.back();

		for(size_t j = 0; j < indices[i].size(); ++j) {
			newVector.insert(indices[i][j]) = vals[i][j];
		}		
	}
}

void SVMUtils::readCsvFile(const char *fileName, std::vector<Eigen::VectorXd> &data, std::vector<double> &labels) {
	ifstream in(fileName);
	//TODO: Check file

	string line, token;
	int lastDim = -1;
	int num0 = 0;
	int num1 = 1;

	while(getline(in, line)) 
	{
		vector<string> tokens;
		istringstream linestream(line);

		while(getline(linestream, token, ',')) {
			tokens.push_back(token);
		}

		int dim = tokens.size() - 1;
		assert(dim == lastDim || lastDim == -1);
		lastDim = dim;

		data.push_back(VectorXd(dim));
		VectorXd &instance = data.back();

		for(int i = 0; i < dim; i++) {
			instance(i) = atof(tokens[i].c_str());
		}

		double label = atof(tokens[dim].c_str());
		if(label == 0.0) {label = -1.0;}

		assert(label == -1.0 || label == 1.0);

		labels.push_back(label);

		if(label == -1.0) {++num0;}
		else {++num1;}
	}

	assert(data.size() == labels.size());
	assert(num0 > 0);
	assert(num1 > 1);
}

