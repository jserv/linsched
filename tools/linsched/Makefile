LINUXDIR = ./../..
LINSCHED_DIR=${LINUXDIR}/tools/linsched
include Makefile.inc

TEST_LIB = test_lib.o

.PHONY: run_all_tests all
.DEFAULT_GOAL := all

all: ${TEST_LIB} ${OBJ_FILES}
	$(MAKE) $(MFLAGS) --directory=tests all
	$(MAKE) $(MFLAGS) --directory=validation all

run_unit_tests: all
	$(MAKE) $(MFLAGS) --directory=tests run_unit_tests

run_all_tests: all
	$(MAKE) $(MFLAGS) --directory=tests run_all_tests

clean:
	rm -f ${TESTS} ${OBJ_FILES} ${DEPS} ${GEN_HDR}
	$(MAKE) $(MFLAGS) --directory=tests clean
