#ifndef _RCD_RCDNODE_H
#define _RCD_RCDNODE_H

#include <Eigen/Dense>

template <class InfoSpec>
class RCDNode {
 public:
	typedef typename InfoSpec::MasterInfo MasterInfo;
	typedef typename InfoSpec::SlaveInfo SlaveInfo;
	typedef typename InfoSpec::NodeInput NodeInput;
	typedef typename InfoSpec::NodeStaticInput NodeStaticInput;
	typedef Eigen::VectorXd Update;

 RCDNode(int varId)
	 : varId_(varId) {}

	virtual ~RCDNode() {}
	virtual int getNumInitPhases() const {return 1;}
	virtual void init(int phase, const NodeStaticInput &staticInput) = 0;
	
	virtual MasterInfo getInfoAsMaster(int slaveId,
									   const NodeInput& input) const = 0;
	virtual void updateAsMaster(
								const MasterInfo &myInfo,
								int slaveId,
								const SlaveInfo &slaveInfo, Update& masterUpdate)
		const = 0;

	virtual void updateAsSlave(
							   int masterId, 
							   const MasterInfo &masterInfo,
							   const NodeInput &input,		
							   SlaveInfo& slaveInfo, Update& slaveUpdate)
		const = 0;

 protected:
	int varId_;
};

#endif
