#!/bin/bash
#for i in `seq 1 4`; do 
i=$1
./fm_part ../testcases/p2-$i.cells ../testcases/p2-$i.nets
#done
