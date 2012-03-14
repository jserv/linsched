#!/bin/bash

# Monitor and produce statistics comparable to
# mcarlo-sim --print_cgroup_stats --print_average_imbalance --print_sched_stats
# and then clean up

BASE_DIR="$(dirname $0)"
DURATION="$1"
TOPO="$2"
MONITOR_CPUS="$3"

CHILDREN=$(pgrep -P $$)
CHILDREN=$(echo $CHILDREN) #squash the lines together
CPUACCT_NAMES=( $(find /dev/cgroup/cpu -name cpuacct.usage) )

# take schedstats delta (primarily for number of load balences)
CPUACCT_BEFORE=( $(cat "${CPUACCT_NAMES[@]}" ) )
cat /proc/schedstat > schedstat.before

# grab trace (of _all_ cpus - cpuset away ones still start load balances)
perf record -a -T -e sched:sched_wakeup -e sched:sched_switch -e sched:sched_migrate_task -- \
    bash -c \
"sleep 1;
kill -CONT $CHILDREN;
sleep $( echo $DURATION / 1000 | bc -l);
kill -STOP $CHILDREN"

CPUACCT_AFTER=( $(cat "${CPUACCT_NAMES[@]}" ) )
cat /proc/schedstat > schedstat.after

echo ------ group runtime
for i in "${!CPUACCT_NAMES[@]}"; do
    name="${CPUACCT_NAMES[$i]}"
    name="${name#/dev/cgroup/cpu}"
    name="${name%/cpuacct.usage}"
    [ -z "$name" ] && name=/
    usage="$(( ${CPUACCT_AFTER[$i]} - ${CPUACCT_BEFORE[$i]} ))"
    echo "CGroup = $name exec_time = $usage"
done | tac
echo ------ sched stats
$BASE_DIR/schedstat-diff.py schedstat.before schedstat.after

perf script -D | grep -v '^#\|^\.' | \
    $BASE_DIR/trace-imbalance $TOPO $MONITOR_CPUS $CHILDREN | tail | tee imbalance
kill -KILL $CHILDREN

# Cleanup
echo 1 > /dev/cgroup/cpuset/cpuset.sched_load_balance
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

if grep 'BUG\|nan' imbalance >& /dev/null; then
    echo error $$ >&2
    exit 1
fi
