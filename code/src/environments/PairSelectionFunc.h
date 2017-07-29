#ifndef _RCD_PAIRSELECTIONFUNC_H
#define _RCD_PAIRSELECTIONFUNC_H

#include <vector>
#include <random>
#include <functional>
#include "environments/NetConfig.h"

class PairSelection {
public:
  virtual ~PairSelection() {}
	virtual int getBufferSize() = 0;
	virtual void getPairs(int out_i[], int out_j[], int &n) = 0;
};

class RandomSinglePair : public PairSelection {
public:
	RandomSinglePair(int numVars, int seed = 0)
		: numVars_(numVars), config_(0), r_(seed) {}

	RandomSinglePair(const NetConfig *config, int seed = 0)
		: numVars_(config->numVars), config_(config), r_(seed) {}
	void getPair(int &i, int &j);
	virtual int getBufferSize() OVERRIDE {return 1;}
	virtual void getPairs(int out_i[], int out_j[], int &n) {
		getPair(out_i[0], out_j[0]);
		n = 1;
	}

private:
	int numVars_;
	const NetConfig *config_;
	std::default_random_engine r_;
};

class RandomKPairs : public PairSelection {
public:
	RandomKPairs(const NetConfig *config, int numPairs, int seed = 0)
		: numVars(config->numVars), numPairs(numPairs), r(seed) {
		adj = config->adjMatrix;
	}

	virtual int getBufferSize() OVERRIDE {return numPairs;}
	virtual void getPairs(int out_i[], int out_j[], int &n) OVERRIDE;

private:
	int numVars;
	int numPairs;
	std::default_random_engine r;
	std::vector<std::vector<int> > adj;
};

class CoveringPairs : public PairSelection {
public:
	CoveringPairs(const NetConfig *config, int seed = 0)
		: numVars(config->numVars), r(seed), startingPoint(0) {
		adj = config->adjMatrix;
	}

	virtual int getBufferSize() OVERRIDE {return numVars / 2;}
	virtual void getPairs(int out_i[], int out_j[], int &n) OVERRIDE;

private:
	int numVars;
	std::default_random_engine r;
	int startingPoint;

	std::vector<std::vector<int> > adj;
};

class OfflinePairs : public PairSelection {
public:
	OfflinePairs()
		: numPairs(0), pair_i(0), pair_j(0), bufSize(-1) {}

	~OfflinePairs() {cleanup();}

	void init(PairSelection *base, int numRecords);
	virtual int getBufferSize() OVERRIDE {return bufSize;}
	virtual void getPairs(int out_i[], int out_j[], int &n) OVERRIDE;
	int getNumRecords() const {return numRecords;}

private:
	OfflinePairs(const OfflinePairs &) {}
	OfflinePairs &operator=(const OfflinePairs &) {return *this;}

	void cleanup();

	int currentRecord;
	int numRecords;

	int *numPairs;
	int **pair_i;
	int **pair_j;
	int bufSize;
};

#endif
