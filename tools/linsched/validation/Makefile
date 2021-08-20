LINUXDIR = ./../../..
LINSCHED_DIR=${LINUXDIR}/tools/linsched
include ../Makefile.inc

UNIX_TESTS := unix-mcarlo-sim trace-imbalance


.DEFAULT_GOAL := all
.PHONY: all

all: ${UNIX_TESTS}

.SECONDEXPANSION:
${UNIX_TESTS}: ${OBJ_FILES} $$@.o
	@echo CC UNIX_TEST $@
	@${LD_PERCPU} -o $@.percpu $^
#two definitions of mktime
	@${CC} -Wl,--allow-multiple-definition -static -o $@ $@.percpu -lrt -lm -lpcre -lpthread
