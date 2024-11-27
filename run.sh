#!/bin/bash

trap ctrl_c INT

function ctrl_c() {
   echo "trapped ctrl-c"
   exit 1
}

./ns3 build
# Produce "all-operations.txt" file which serves to display the accessed MCS by each manager. Compare against MCS table to verify.
# Output is appended to this file so if you are trying different experiments make sure to delete file
echo -n "Producing data for all MCS ... "
python3 scripts/simul.py scripts/all-operation.yaml
echo "done"

# Produce Figure 5
echo -n "Producing Figure 5 ... "
python3 scripts/simul-remy.py scripts/distance.yaml > distance-output.txt
python3 scripts/extract-distance.py distance-output.txt
echo "done"


#Produce all plots in Figure 6 (See .eps files for output)
echo -n "Producing Figure 6 ... "
./ns3 run --no-build 'convergenceShape --standard=802.11ac --staticStep=true --serverShortGuardInterval=800 --clientShortGuardInterval=800 --serverNss=1 --clientNss=1 --serverChannelWidth=20 --clientChannelWidth=20 --wifiManager=Ideal'
./ns3 run --no-build 'convergenceShape --standard=802.11ac --staticStep=true --serverShortGuardInterval=800 --clientShortGuardInterval=800 --serverNss=1 --clientNss=1 --serverChannelWidth=20 --clientChannelWidth=20 --wifiManager=MinstrelHt'
./ns3 run --no-build 'convergenceShape --standard=802.11ac --staticStep=true --serverShortGuardInterval=800 --clientShortGuardInterval=800 --serverNss=1 --clientNss=1 --serverChannelWidth=20 --clientChannelWidth=20 --wifiManager=ThompsonSampling'
gnuplot *plt
echo "done"

#Produce all plots in Figure 8 (Fig.A=drop.png, Fig.B=up.png, Fig.C=drop2.png, Fig.D=up2.png)
echo -n "Producing Figure 8 ... "
python3 scripts/simul.py scripts/drop.yaml
python3 scripts/simul.py scripts/drop-2.yaml
python3 scripts/simul.py scripts/drop-3.yaml
python3 scripts/simul.py scripts/drop-4.yaml
python3 scripts/simul.py scripts/drop-5.yaml
python3 scripts/simul.py scripts/up.yaml
python3 scripts/simul.py scripts/up-2.yaml
python3 scripts/simul.py scripts/up-3.yaml
python3 scripts/simul.py scripts/up-4.yaml
python3 scripts/simul.py scripts/up-5.yaml
python3 scripts/convergence.py
echo "done"
