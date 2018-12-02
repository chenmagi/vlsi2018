#!/bin/bash
mkdir -p .seedgen
export hw3_time_limit=1
if [ $# -ge 2 ]; then ratio=$2; else ratio=0.1; fi
mkdir -p .seedgen/n$1
for i in `seq 1 100`; do 
rand=$RANDOM
export hw3_seed=$rand
echo "Iteration "$i ", rand="$rand
./seed_gen ../testcase/n$1.hardblocks ../testcase/n$1.nets ../testcase/n$1.pl .log/n$1.floorplan $ratio > .seedgen/n$1/r$rand.log
done
unset hw3_time_limit

