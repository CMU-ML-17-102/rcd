#ifndef _RCD_PARAMETERCLIENT_H
#define _RCD_PARAMETERCLIENT_H

#include <string>
#include <Eigen/Dense>
#include "core/Platform.h"

// Default static input needed by node x_i is A_i and L_i
struct DefaultStaticInput {
	const Eigen::MatrixXd *A;
	double L;
};

// A paramater read client is an interface to retrieve
// information required to update parameters.
// The client provides two categories of information:
// NodeStaticInput - information that does not change per iteration
// such as constraint submatrix and Lipschitz constant.
// NodeInput - information that can change per iteration
// such as parameter values.
template<class NodeInput,
	class NodeStaticInput = DefaultStaticInput>
class ParameterReadClient {
 public:
	virtual ~ParameterReadClient() {}
	virtual void getNodeInput(int varId, NodeInput &nodeInput) = 0;
	virtual NodeStaticInput getNodeStaticInput(int varId) = 0;
};

class ParameterUpdateClient {
 public:
	virtual ~ParameterUpdateClient() {}

	virtual void update(int varId1, int varId2,
			const Eigen::VectorXd& increment1,
			const Eigen::VectorXd& increment2, bool async) = 0;

	virtual void init(int varId) = 0;
};

#endif
