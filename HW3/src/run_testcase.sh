#!/bin/bash
mkdir -p .log

if [ $# -ge 2 ]; then ratio=$2; else ratio=0.1; fi
./hw3 ../testcase/n$1.hardblocks ../testcase/n$1.nets ../testcase/n$1.pl .log/n$1.floorplan $ratio

