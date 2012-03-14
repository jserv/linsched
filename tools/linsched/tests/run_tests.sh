#!/bin/bash

#runs all tests given to it, stopping on the first unexpected failure
#knows to run subtests for basic_tests

function die() {
  echo $1
  exit 1
}

for i in "$@"; do
  if [ "$i" = "basic_tests" ]; then
    for a in $(./basic_tests list); do
      echo "Running test $a"
      for topo in "uniprocessor" "dual_cpu" "dual_cpu_mc" "quad_cpu" "quad_cpu_mc" "quad_cpu_dual_socket" "quad_cpu_quad_socket" "hex_cpu_dual_socket_smt"; do
        echo -ne "Testing topology $topo ....."
        ./basic_tests $a $topo
	if [ $? -eq 0 ]; then
		echo "PASSED"
	else
		echo "'basic_tests $a $topo' FAILED"
	fi
      done
    done
  else
    ./$i || die "$i failed"
  fi
done
