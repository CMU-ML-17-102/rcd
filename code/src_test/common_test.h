#ifndef _RCD_COMMON_TEST_H_
#define _RCD_COMMON_TEST_H_

#include <cassert>

#define ASSERT_WITHIN(x, lower, upper) assert((x) >= (lower) && (x) <= (upper))
#define ASSERT_NEAR(x, target, tolerance) ASSERT_WITHIN((x), (target)-(tolerance), \
														(target)+(tolerance))
#endif
