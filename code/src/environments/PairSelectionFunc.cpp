#include <cassert>
#include <algorithm>
#include <iostream>
#include "environments/PairSelectionFunc.h"

using namespace std;

void RandomSinglePair::getPair(int &vi, int &vj) {	
	std::uniform_int_distribution<int> ui(0, numVars_ - 1);
	vi = ui(r_);

	if(config_) {
		std::uniform_int_distribution<int> uj(0, config_->adjMatrix[vi].size() - 1);
		vj = config_->adjMatrix[vi][uj(r_)];
	} else { //Asssume clique
		std::uniform_int_distribution<int> uj(0, numVars_ - 2);
		vj = uj(r_);		
		if(vj >= vi) {++vj;}
	}
}

void RandomKPairs::getPairs(int out_i[], int out_j[], int &n) {
	n = numPairs;

	int numNeighbors[numVars];
	for(int i = 0; i < numVars; i++) {numNeighbors[i] = adj[i].size();}

	int idx[numVars]; //mapping from pool index to node id
	int backIdx[numVars]; //mapping from node id to pool index
	for(int k = 0; k < numVars; k++) {backIdx[k] = idx[k] = k;}

	for(int k = 0; k < numPairs; k++) {
		int idx_i = -1, vi = -1;
		int lastIdx = numVars - 1 - 2*k;

		//Select vi
		do {
			std::uniform_int_distribution<int> ui(0, lastIdx);
			idx_i = ui(r);
			vi = idx[idx_i];
		}while(numNeighbors[vi] == 0);

		//Select vj from neighbors
		std::uniform_int_distribution<int> uj(0, numNeighbors[vi] - 1);
		int vj = adj[vi][uj(r)];
		int idx_j = backIdx[vj];

		//Remove vi from other nodes
		for(int h = 0; h < numNeighbors[vi]; h++) {
			int v = adj[vi][h];
			auto pos = find(adj[v].begin(), adj[v].begin() + numNeighbors[v], vi);
			int tmp = *pos;
			*pos = adj[v][numNeighbors[v] - 1];
			adj[v][numNeighbors[v] - 1] = tmp;
			--numNeighbors[v];
		}

		//Remove vj from other nodes
		for(int h = 0; h < numNeighbors[vj]; h++) {
			int v = adj[vj][h];
			auto pos = find(adj[v].begin(), adj[v].begin() + numNeighbors[v], vj);
			int tmp = *pos;
			*pos = adj[v][numNeighbors[v] - 1];
			adj[v][numNeighbors[v] - 1] = tmp;
			--numNeighbors[v];
		}

		//Output vi & vj
		out_i[k] = vi;
		out_j[k] = vj;

		//Remove vi and vj from pool
		assert(idx_i <= lastIdx);
		if(backIdx[vi] != idx_i) {std::cerr << backIdx[vi] << "!=" << idx_i << std::endl;}
		assert(backIdx[vi] == idx_i);
		assert(backIdx[vj] <= lastIdx);

		idx[idx_i] = idx[lastIdx];
		backIdx[idx[idx_i]] = idx_i;
		idx_j = backIdx[idx[idx_j]];	//handle the case where idx_j = lastIdx
		idx[idx_j] = idx[lastIdx-1];
		backIdx[idx[idx_j]] = idx_j;
	}
}

void CoveringPairs::getPairs(int out_i[], int out_j[], int &n) {
	n = 0;

	bool selected[numVars];
	std::fill(selected, selected + numVars, false);
	int numNeighbors[numVars];
	for(int i = 0; i < numVars; i++) {numNeighbors[i] = adj[i].size();}

	for(int i = 0; i < numVars; i++) {
		int vi = (startingPoint + i) % numVars;
		if(selected[vi]) {continue;}

		if(numNeighbors[vi] == 0) {continue;}

		std::uniform_int_distribution<int> uj(0, numNeighbors[vi] - 1);
		int vj = adj[vi][uj(r)];
		assert(!selected[vj]);
	
		//Output vi & vj
		out_i[n] = vi;
		out_j[n++] = vj;
		selected[vi] = selected[vj] = true;

		//Remove vi from other nodes
		for(int h = 0; h < numNeighbors[vi]; h++) {
			int v = adj[vi][h];
			auto pos = find(adj[v].begin(), adj[v].begin() + numNeighbors[v], vi);
			int tmp = *pos;
			*pos = adj[v][numNeighbors[v] - 1];
			adj[v][numNeighbors[v] - 1] = tmp;
			--numNeighbors[v];
		}

		//Remove vj from other nodes
		for(int h = 0; h < numNeighbors[vj]; h++) {
			int v = adj[vj][h];
			auto pos = find(adj[v].begin(), adj[v].begin() + numNeighbors[v], vj);
			int tmp = *pos;
			*pos = adj[v][numNeighbors[v] - 1];
			adj[v][numNeighbors[v] - 1] = tmp;
			--numNeighbors[v];
		}
	}
	
	startingPoint = (startingPoint + 1) % numVars;
}

void OfflinePairs::init(PairSelection *base, int numRecords) {
	//cerr << "Creating offline samples" << endl;
	cleanup();

	currentRecord = 0;
	this->numRecords = numRecords;
	bufSize = base->getBufferSize();

	numPairs = new int[numRecords];
	pair_i = new int*[numRecords];
	pair_j = new int*[numRecords];

	for(int i = 0; i < numRecords; i++) {
		pair_i[i] = new int[bufSize];
		pair_j[i] = new int[bufSize];
		base->getPairs(pair_i[i], pair_j[i], numPairs[i]);

		//if(i % 100 == 0) {
		//	cerr << "Finished " << i << " samples" << endl;
		//}
	}
}

void OfflinePairs::getPairs(int out_i[], int out_j[], int &n) {
	n = numPairs[currentRecord];
	std::copy(pair_i[currentRecord], pair_i[currentRecord] + numPairs[currentRecord], out_i);
	std::copy(pair_j[currentRecord], pair_j[currentRecord] + numPairs[currentRecord], out_j);
	currentRecord = (currentRecord + 1) % numRecords;
}

void OfflinePairs::cleanup() {
	if(numPairs) {
		for(int i = 0 ; i < numRecords; i++) {
			delete pair_i[i];
			delete pair_j[i];
		}

		delete pair_i;
		delete pair_j;
		delete numPairs;
	}
}
