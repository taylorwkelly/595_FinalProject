#!/bin/bash

trap ctrl_c INT

function ctrl_c() {
   echo "trapped ctrl-c"
   exit 1
}

./ns3 build
echo -n "Generating pathing data ... "
python3 scripts/generate-figure-4.py scripts/packet_size.yaml > packet-loss-output.txt
echo -n "plotting ... "
python3 scripts/packet_loss_graphing.py packet-loss-output.txt
echo "done"
