#!/bin/bash

trap ctrl_c INT

function ctrl_c() {
   echo "trapped ctrl-c"
   exit 1
}

./ns3 build
# echo -n "Generating Figure 4 data ... "
# python3 scripts/generate-figure-4.py scripts/figure-4-configuration.yaml > dist_t_testing.txt
echo -n "plotting ... "
python3 scripts/distance_testing.py d_t_testing.txt
echo "done"
