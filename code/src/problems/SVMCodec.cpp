#ifdef USE_PBUF

#include "problems/SVMCodec.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include "problems/SVMProtos.pb.h"

void SVMCodec::encodeNodeInput(const SVMNodeInput& input, std::string &codedInput) {
	SVMInputProto proto;
	int size = input.numVars;
	proto.set_numvars(size);
	proto.mutable_alpha()->mutable_elements()->Reserve(size);

	for(int i = 0; i < size; ++i) {
		proto.mutable_alpha()->mutable_elements()->Add(input.alpha[i][0]);
	}

	codedInput = proto.SerializeAsString();
	proto.PrintDebugString();
}

void SVMCodec::decodeNodeInput(const std::string &codedInput, SVMNodeInput &input) {
	SVMInputProto proto;
	proto.ParseFromString(codedInput);
	int size = proto.numvars();
	Eigen::VectorXd *alpha = new Eigen::VectorXd[size];

	for(int i = 0; i < size; ++i) {
		alpha[i].resize(1);
		alpha[i][0] = proto.alpha().elements(i);
	}

	input.alpha = alpha;
}

void SVMCodec::encodeNodeStaticInput(const SVMStaticInput& input, std::string &codedInput) {
	SVMStaticInputProto proto;
	int size = input.numVars;
	proto.mutable_y()->mutable_elements()->Reserve(size);
	//double *dst = proto.mutable_y()->mutable_elements()->mutable_data();
	//const double *src = input.y;
	//std::copy(src, src + size, dst);
	//proto.mutable_y()->mutable_elements()->AddAlreadyReserved();
	for(int i = 0; i < size; ++i) {
		proto.mutable_y()->add_elements(input.y[i]);
	}

	proto.set_numvars(size);
	proto.set_c(input.C);
	codedInput = proto.SerializeAsString();
	proto.PrintDebugString();
}

void SVMCodec::decodeNodeStaticInput(const std::string &codedInput, SVMStaticInput &input) {
	SVMStaticInputProto proto;
	proto.ParseFromString(codedInput);
	int size = proto.numvars();
	input.numVars = size;
	input.C = proto.c();
	double *y = new double[size];
	const double *src = proto.y().elements().data();
	std::copy(src, src + size, y);
	input.y = y;
}

#endif
