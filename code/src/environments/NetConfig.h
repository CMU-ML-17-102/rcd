#ifndef _RCD_NETCONFIG_H_
#define _RCD_NETCONFIG_H_

#include<vector>

class NetConfig {
public:
	NetConfig(int numVars)
		: numVars(numVars), adjMatrix(numVars)
	{}

	int numVars;
	std::vector<std::vector<int> > adjMatrix;

	static NetConfig createChain(int numVars) {
		NetConfig config(numVars);
		
		config.adjMatrix[0].push_back(1);
		for(int i = 1; i < numVars-1; i++) {
			config.adjMatrix[i].push_back(i-1);		
			config.adjMatrix[i].push_back(i+1);
		}
		config.adjMatrix[numVars-1].push_back(numVars-2);

		return config;
	}

	static NetConfig createRing(int numVars) {
		NetConfig config(numVars);
		
		config.adjMatrix[0].push_back(1);
		//config.adjMatrix[0].push_back(numVars-1);
		
		for(int i = 1; i < numVars-1; i++) {
			config.adjMatrix[i].push_back(i-1);		
			config.adjMatrix[i].push_back(i+1);
		}

		//config.adjMatrix[numVars-1].push_back(0);		
		config.adjMatrix[numVars-1].push_back(numVars-2);

		return config;
	}

	static NetConfig createClique(int numVars) {
		NetConfig config(numVars);
		
		for(int i = 0; i < numVars; i++) {
			for(int j = i+1; j < numVars; j++) {
				config.adjMatrix[i].push_back(j);
				config.adjMatrix[j].push_back(i);
			}
		}

		return config;
	}

	static NetConfig createTree(int numVars) {
		NetConfig config(numVars);
		
		for(int i = 0; i <= numVars/2; i++) {
			for(int j = 2*i+1; j <= 2*i+2; j++) {
				if(j < numVars) {
					config.adjMatrix[i].push_back(j);
					config.adjMatrix[j].push_back(i);
				}
			}
		}

		return config;
	}

	static NetConfig createStar(int numVars) {
		NetConfig config = createRing(numVars);
		
		for(int i = 2; i < numVars - 1; i++) {
			config.adjMatrix[0].push_back(i);
			config.adjMatrix[i].push_back(0);
		}

		return config;
	}

	static NetConfig createSuperTree(int numVars) {
		NetConfig config = createRing(numVars);
		
		for(int i = 0; i <= numVars/2; i++) {
			for(int j = 2*i+1; j <= 2*i+2; j++) {
				if(j < numVars && j-i > 1) {
					config.adjMatrix[i].push_back(j);
					config.adjMatrix[j].push_back(i);
				}
			}
		}

		return config;
	}

};

#endif

