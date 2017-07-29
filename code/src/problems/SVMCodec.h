#ifndef RCD_SVMCODEC_H_
#define RCD_SVMCODEC_H_

#include "problems/SVMProblem.h"

class SVMCodec {
public:
	static void encodeNodeInput(const SVMNodeInput& input, std::string &codedInput);
	static void decodeNodeInput(const std::string &codedInput, SVMNodeInput &input);
	static void encodeNodeStaticInput(const SVMStaticInput& input, std::string &codedInput);
	static void decodeNodeStaticInput(const std::string &codedInput, SVMStaticInput &input);

private:
	SVMCodec() = delete;
};

#endif
