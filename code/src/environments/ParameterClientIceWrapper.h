#ifndef _RCD_PARAMETERCLIENTICEWRAPPER_H
#define _RCD_PARAMETERCLIENTICEWRAPPER_H
#ifdef USE_ICE

#include <Ice/Ice.h>
#include "core/Problem.h"
#include "environments/ParameterClientIce.h"

class ParameterUpdateIceServer : public rcd::ParameterUpdateIce {
public:
	ParameterUpdateIceServer(ParameterUpdateClient *localClient, bool ownLocalClient = true)
		: localClient_(localClient), ownLocalClient_(ownLocalClient) {}

	~ParameterUpdateIceServer() {
		if(ownLocalClient_) { delete localClient_; }
	}

	virtual void update(int varId1, int varId2,
						const std::pair<const double *, const double *> &update1,
						const std::pair<const double *, const double *> &update2,
						bool async, const Ice::Current &) OVERRIDE {
		int varDim = update1.second - update1.first;

		Eigen::Map<Eigen::VectorXd> update1Vector(const_cast<double *>(update1.first), varDim);
		Eigen::Map<Eigen::VectorXd> update2Vector(const_cast<double *>(update2.first), varDim);
		localClient_->update(varId1, varId2, update1Vector, update2Vector, async);
	}

	virtual void init(int varId, const Ice::Current &) OVERRIDE {
		localClient_->init(varId);
	}

	void startServer(Ice::CommunicatorPtr ic, const char *protocolString,
					 const char *adapterName, const char *servantName) {
		Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints(
										adapterName, protocolString);
		Ice::ObjectPtr object = this;
		adapter->add(object, ic->stringToIdentity(servantName));
		adapter->activate();
	}

private:
	ParameterUpdateClient *localClient_;
	bool ownLocalClient_;
};

class ParameterUpdateIceClient : public ParameterUpdateClient {
public:
	void connect(Ice::CommunicatorPtr ic, const char *connectionString) {
		Ice::ObjectPrx base = ic->stringToProxy(connectionString);
		proxy_ = rcd::ParameterUpdateIcePrx::checkedCast(base);
		if (!proxy_) {throw "Invalid proxy";}
	}

	virtual void update(int varId1, int varId2,
			const Eigen::VectorXd& increment1,
			const Eigen::VectorXd& increment2, bool async) OVERRIDE {
		int varDim = increment1.size();
		std::pair<const double *, const double *> update1(increment1.data(), increment1.data() + varDim);
		std::pair<const double *, const double *> update2(increment2.data(), increment2.data() + varDim);
		proxy_->update(varId1, varId2, update1, update2, async);
	}

	virtual void init(int varId) OVERRIDE {
		proxy_->init(varId);
	}

private:
	rcd::ParameterUpdateIcePrx proxy_;
};

template<class NodeInput, class NodeStaticInput, class Codec>
class ParameterReadIceServer : public rcd::ParameterReadIce {
public:
	ParameterReadIceServer(ParameterReadClient<NodeInput, NodeStaticInput> *localClient,
						   bool ownLocalClient = true)
		: localClient_(localClient), ownLocalClient_(ownLocalClient) {}

	~ParameterReadIceServer() {
		if(ownLocalClient_) { delete localClient_; }
	}

	virtual void getNodeInput(int varId, std::string &codedInput, const Ice::Current &) OVERRIDE {
		NodeInput input;
		localClient_->getNodeInput(varId, input);
		Codec::encodeNodeInput(input, codedInput);
	}

	virtual void getNodeStaticInput(int varId, std::string &codedInput, const Ice::Current &) OVERRIDE {
		NodeStaticInput input = localClient_->getNodeStaticInput(varId);
		Codec::encodeNodeStaticInput(input, codedInput);
	}

	void startServer(Ice::CommunicatorPtr ic, const char *protocolString,
		 const char *adapterName, const char *servantName) {
		Ice::ObjectAdapterPtr adapter = ic->createObjectAdapterWithEndpoints(
										adapterName, protocolString);
		Ice::ObjectPtr object = this;
		adapter->add(object, ic->stringToIdentity(servantName));
		adapter->activate();
	}

private:
	ParameterReadClient<NodeInput, NodeStaticInput> *localClient_;
	bool ownLocalClient_;
};

template<class NodeInput, class NodeStaticInput, class Codec>
class ParameterReadIceClient : public ParameterReadClient<NodeInput, NodeStaticInput> {
public:
	void connect(Ice::CommunicatorPtr ic, const char *connectionString) {
		Ice::ObjectPrx base = ic->stringToProxy(connectionString);
		proxy_ = rcd::ParameterReadIcePrx::checkedCast(base);
		if (!proxy_) {throw "Invalid proxy";}
	}

	virtual void getNodeInput(int varId, NodeInput &input) OVERRIDE {
		std::string codedInput;
		proxy_->getNodeInput(varId, codedInput);
		Codec::decodeNodeInput(codedInput, input);
	}

	virtual NodeStaticInput getNodeStaticInput(int varId) OVERRIDE {
		std::string codedInput;
		proxy_->getNodeStaticInput(varId, codedInput);
		NodeStaticInput input;
		Codec::decodeNodeStaticInput(codedInput, input);
		return input;
	}

private:
	rcd::ParameterReadIcePrx proxy_;
};

template<class InfoSpec, class Codec>
class ParameterIceServer {
	typedef ParameterReadIceServer<typename InfoSpec::NodeInput,
								   typename InfoSpec::NodeStaticInput,Codec> ReadServer;
	typedef ParameterUpdateIceServer UpdateServer;

public:
	ParameterIceServer(Problem<InfoSpec> *problem) {
		updateServer_ = new UpdateServer(problem->createLocalParameterUpdateClient());
		readServer_ = new ReadServer(problem->createLocalParameterReadClient());
	}

	void startServer(Ice::CommunicatorPtr ic, const char *readProtocolString,
			 const char *updateProtocolString,
			 const char *readServantName, const char *updateServantName,
			 const char *readAdapterName = "ParamReadAdapter",
			 const char *updateAdapterName = "ParamUpdateAdapter") {
		updateServer_->startServer(ic, updateProtocolString, updateAdapterName, updateServantName);
		readServer_->startServer(ic, readProtocolString, readAdapterName, readServantName);
	}

private:
	UpdateServer *updateServer_;
	ReadServer *readServer_;
};

#endif
#endif
