#!/bin/bash

#Produce all plots in Figure 5 (See .eps files for output)
echo -n "Generating Figure 5 data and plots ... "
./ns3 run --no-build 'figure-5 --standard=802.11ac --serverShortGuardInterval=800 --clientShortGuardInterval=800 --serverNss=1 --clientNss=1 --serverChannelWidth=20 --clientChannelWidth=20 --wifiManager=Ideal'
gnuplot figure-5-Ideal.plt
./ns3 run --no-build 'figure-5 --standard=802.11ac --serverShortGuardInterval=800 --clientShortGuardInterval=800 --serverNss=1 --clientNss=1 --serverChannelWidth=20 --clientChannelWidth=20 --wifiManager=MinstrelHt'
gnuplot figure-5-MinstrelHt.plt
./ns3 run --no-build 'figure-5 --standard=802.11ac --serverShortGuardInterval=800 --clientShortGuardInterval=800 --serverNss=1 --clientNss=1 --serverChannelWidth=20 --clientChannelWidth=20 --wifiManager=ThompsonSampling'
gnuplot figure-5-ThompsonSampling.plt
echo "done"
