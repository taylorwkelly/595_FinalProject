#!/bin/bash

trap ctrl_c INT

function ctrl_c() {
   echo "trapped ctrl-c"
   exit 1
}

./ns3 build
echo -n "Generating Figure 4 data ... "
python3 scripts/generate-figure-4.py scripts/figure-4-configuration.yaml > figure-4-output.txt
echo -n "plotting ... "
python3 scripts/plot-figure-4.py figure-4-output.txt
echo "done"
