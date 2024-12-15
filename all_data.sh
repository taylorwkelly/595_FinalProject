#!/bin/bash

trap ctrl_c INT

function ctrl_c() {
   echo "trapped ctrl-c"
   exit 1
}

./ns3 build
echo -n "Generating all data ... "
python3 scripts/generate-figure-4.py scripts/full-output.yaml > full-output.txt
echo -n "plotting ... "
python3 scripts/full-output.py full-output.txt
echo "done"
