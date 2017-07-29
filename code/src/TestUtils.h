#ifndef _RCD_TESTUTILS_H_

#define _RCD_TESTUTILS_H_

#include <vector>
#include "core/RCDScheduler.h"
#include "problems/SepSmoothObjectiveProblem.h"

enum Layout {
	LAYOUT_CHAIN = 0,
	LAYOUT_RING = 1,
	LAYOUT_CLIQUE = 2,
	LAYOUT_TREE = 3,
	LAYOUT_STAR = 4,
	LAYOUT_SUPERTREE = 5
};

enum ConstraintType {
	CONS_IDENTITY = 0,
	CONS_RANDOM_NONNEG = 1
};

enum ScheduleType {
	SCHED_SPIN_DOUBLE = 0,
	SCHED_SPIN_SINGLE = 1,
	SCHED_LOCK_FREE = 2
};

struct Test {
	Test()
	: condition(0.0), maxIterations(100000)
	, constraint(CONS_RANDOM_NONNEG) 
	, schedule(SCHED_SPIN_DOUBLE), eps(-1)
	, syncPeriod(-1), stopping(CHANGE) {}

	int numVars;
	int varDim;
	int numConstraints;
	Layout layout;
	const char *outputFile;
	double condition;
	int maxIterations;
	ConstraintType constraint;
	ScheduleType schedule;
	double eps;
	int syncPeriod;
	StoppingCondition stopping;
};

SepSmoothObjectiveProblem createQuadFunctions(int numVars, int varDim, int start, int end);
OptOutput quickTest(const Test &test);

#endif
