#ifndef _RCD_COMMON_H_
#define _RCD_COMMON_H_

#include <cassert>
#include <iostream>

#define OVERRIDE override

#define LOG(x) std::cerr << "LOG:" << x << std::endl

#define ASSERT(condition, msg) {if(!(condition)) {std::cerr << "ASSEERTION FAILED: " << #condition << std::endl << "MESSAGE: " << msg << std::endl; assert(false);}}

#endif
