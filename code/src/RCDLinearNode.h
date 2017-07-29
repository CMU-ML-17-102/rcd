#ifndef _RCD_RCDLINEARNODE_H
#define _RCD_RCDLINEARNODE_H

#include <atomic>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include "SpinLock.h"
#include "RCDScheduler.h"
#include "SmoothObjectiveProblem.h"

class NetConfig;

/** 
 * Cache to store pair matrices [pinv(Ai Ai' / Li + Aj Aj' / Lj)]
 * to avoid recomputation. 
 * 
 * Cache access is lock-free but that requires all cache entries to be preallocated.
 * Once an entry is assigned a value, it will not be overwritten. The assumption is
 * that overwriting attempts are due to multiple threads computing the same value.
 */ 
//TODO: Make it a template
class SmoothObjCache {
 public:
	~SmoothObjCache() {
		for(auto it = data.begin(); it != data.end(); ++it) {
			for(auto it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
				if(it2->second) {delete it2->second;}
			}
		}
	}

	void allocateEntry(int id1, int id2, const Eigen::MatrixXd *matrix = 0) {
		lock.lock();
		sortIds(id1, id2);
		data[id1][id2] = matrix;
		lock.unlock();
	}

	/**
     * Updates the entry corresponding to nodes given by id1 and id2. 
	 * If the entry is empty it is set to "matrix" and the cache
	 * takes ownership of "matrix" and the function returns the same pointer. 
	 * If the entry already has a value, "matrix" is deleted and the function
	 * returns the pointer stored in that entry.
     */
	const Eigen::MatrixXd *updateEntry(int id1, int id2, const Eigen::MatrixXd *matrix);
	const Eigen::MatrixXd *readEntry(int id1, int id2) const;
	static SmoothObjCache defaultCache; //TODO: Is this a good idea ?

 private:
	static void sortIds(int &id1, int &id2) {
		if(id1 > id2) {
			int tmp = id1;
			id1 = id2;
			id2 = tmp;
		}
	}

	std::unordered_map<int, std::unordered_map<int, const Eigen::MatrixXd *> > data;
	SpinLock lock;
};

/**
 * RCDNode subclass for generic smooth objective with linear constraints.
 */
class SepSmoothObjNode : public RCDNode<SepSmoothObjInfoSpec> {
	typedef RCDNode<SepSmoothObjInfoSpec> Super; 
 public:
 SmoothObjNode(int varId, NetConfig *config, int varDim, 
			   RCDScheduler<SepSmoothObjInfoSpec> *scheduler,
			   SmoothObjCache &cache = SmoothObjCache::defaultCache)
	 : Super(varId), varDim(varDim), 
		config(config), cache(cache), precomputePairMatrices(true),
		scheduler(scheduler) {}

	virtual int getNumInitPhases() const OVERRIDE {return precomputePairMatrices ?2 :1;}
	virtual void init(int phase, const DefaultStaticInput &nodeInput) OVERRIDE;

	virtual SmoothObjInfo getInfoAsMaster(int slaveId,
												const DefaultNodeInput& input)
		const OVERRIDE;

	virtual void updateAsMaster(
								const SmoothObjMasterInfo &myInfo,
								int slaveId,
								const SmoothObjSlaveInfo &slaveInfo,
								Eigen::VectorXd& masterUpdate) const OVERRIDE;

	virtual void updateAsSlave(int masterId,
							   const SmoothObjMasterInfo &masterInfo,
							   const DefaultNodeInput &input,
							   SmoothObjSlaveInfo& slaveInfo,
							   Eigen::VectorXd& slaveUpdate) const;

 private:
	static void checkConstraintRows(const Eigen::MatrixXd &A, int varDim);
	static void pinv(const Eigen::MatrixXd &m, Eigen::MatrixXd &out);

	int varDim;
	double L;
	const Eigen::MatrixXd *A;
	Eigen::MatrixXd AAT;
	const NetConfig *config;
	SmoothObjCache &cache;
	bool precomputePairMatrices;
	RCDScheduler<SepSmoothObjInfoSpec> *scheduler;

	static SmoothObjCache defaultCache;
};

#endif
