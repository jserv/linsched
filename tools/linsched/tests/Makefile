LINUXDIR = ./../../..
LINSCHED_DIR=${LINUXDIR}/tools/linsched

include ../Makefile.inc

RAND_TEST_TYPES = rand gaussian poisson

UNIT_TESTS = linsched_rand_test

PERFORMANCE_TESTS = linsched basic_tests batch_balance_test \
		linsched_rnd_dist mcarlo-sim

TESTS = ${UNIT_TESTS} ${PERFORMANCE_TESTS} fractional_cpu_test \
	fractional_cpu_test_rnd_dist perf_replay

.DEFAULT_GOAL := all
.PHONY: run_all_tests all

all: ${TESTS}

run_unit_tests: ${UNIT_TESTS}
	( ulimit -s 8192; ./run_unit_tests.sh ${RAND_TEST_TYPES} )

run_all_tests: ${PERFORMANCE_TESTS}
# make seems to remove stack size ulimits for no apparent reason,
# and stack overflow due to recursion is a plausible bug
	( ulimit -s 8192; ./run_tests.sh $^ )

TEST_DEPS := ${TESTS:%=%.d}
-include ${TEST_DEPS}

.SECONDEXPANSION:
${TESTS}: ${OBJ_FILES} $$@.o
	@echo CC TEST $@
	@${LD_PERCPU} -o $@.percpu $^
	@${CC} ${LFLAGS} -o $@ $@.percpu -MMD
	@rm $@.percpu

clean:
	rm -f ${TESTS} ${TEST_DEPS} *.o
