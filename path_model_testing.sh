#!/bin/bash

trap ctrl_c INT

function ctrl_c() {
   echo "trapped ctrl-c"
   exit 1
}

./ns3 build
echo -n "Generating pathing data ... "
python3 scripts/generate-figure-4.py scripts/path_testing.yaml > path-output.txt
echo -n "plotting ... "
python3 scripts/path_testing.py path-output.txt
echo "done"
