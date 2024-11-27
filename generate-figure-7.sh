#!/bin/bash

trap ctrl_c INT

function ctrl_c() {
   echo "trapped ctrl-c"
   exit 1
}

./ns3 build
echo -n "Producing Figure 7a ... "
python3 scripts/generate-figure-7.py scripts/figure-7a.yaml
python3 scripts/plot-figure-7.py figure-7a
echo -n "Figure 7b ... "
python3 scripts/generate-figure-7.py scripts/figure-7b.yaml
python3 scripts/plot-figure-7.py figure-7b
echo -n "Figure 7c ... "
python3 scripts/generate-figure-7.py scripts/figure-7c.yaml
python3 scripts/plot-figure-7.py figure-7c
echo -n "Figure 7d ... "
python3 scripts/generate-figure-7.py scripts/figure-7d.yaml
python3 scripts/plot-figure-7.py figure-7d
echo "done"
