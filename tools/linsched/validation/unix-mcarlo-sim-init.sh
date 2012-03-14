#!/bin/bash

# Cgroup initialization for unix-mcarlo-sim, because the file
# manipulation would be painful in C.

die () {
    [ -n "$*" ] && echo "$@" >&2
    echo 'cgroup initialization failed' >&2
    exit 1
}

move_tasks () {
    from=$1
    to=$2
    for t in $(cat $from/tasks); do
        echo $t > $to/tasks 2>/dev/null
    done
}

for h in /dev/cgroup/*; do
    find $h -mindepth 1 -depth -type d | while read cg; do
        move_tasks $cg $h
        rmdir $cg 2> /dev/null
    done
done

umount /dev/cgroup/*
rmdir /dev/cgroup/* 2> /dev/null
rm -rf /dev/cgroup/* 2> /dev/null
mkdir /dev/cgroup/cpu /dev/cgroup/cpuset
mount -t cgroup -o cpu,cpuacct none /dev/cgroup/cpu || die
mount -t cgroup -o cpuset none /dev/cgroup/cpuset || die

mkdir /dev/cgroup/cpuset/test /dev/cgroup/cpuset/monitor 2>/dev/null
[ -f /dev/cgroup/cpuset/test/cpuset.cpus -a \
  -f /dev/cgroup/cpuset/monitor/cpuset.cpus ] || die "cpuset cgroups failed"

echo $1 > /dev/cgroup/cpuset/test/cpuset.cpus || die
echo $2 > /dev/cgroup/cpuset/monitor/cpuset.cpus || die
cat /dev/cgroup/cpuset/cpuset.mems > /dev/cgroup/cpuset/monitor/cpuset.mems || die
cat /dev/cgroup/cpuset/cpuset.mems > /dev/cgroup/cpuset/test/cpuset.mems || die

move_tasks /dev/cgroup/cpuset /dev/cgroup/cpuset/monitor
echo 0 > /dev/cgroup/cpuset/cpuset.sched_load_balance || die
echo 1 > /dev/cgroup/cpuset/test/cpuset.sched_load_balance || die
