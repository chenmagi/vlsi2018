#!/bin/bash
for i in `seq 1 100`; do
rand=$RANDOM
echo "iteration "$i ", random="$rand
./hw3 ../testcase/n100.hardblocks ../testcase/n100.nets ../testcase/n100.pl $rand>../results/r$rand.log
done

