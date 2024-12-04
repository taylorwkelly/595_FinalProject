#!/bin/bash

# Function to handle Ctrl+C interruptions
trap ctrl_c INT

function ctrl_c() {
   echo "trapped ctrl-c"
   exit 1
}

echo "Building ns-3..."
./ns3 build || { echo "Build failed. Exiting."; exit 1; }

echo "Generating SNR vs Latency data..."
python3 scripts/generate-figure-7.py scripts/figure-7a.yaml || { echo "Data generation failed. Exiting."; exit 1; }

echo "Generating SNR vs Latency plot..."
python3 scripts/plot-figure-7.py || { echo "Plot generation failed. Exiting."; exit 1; }

echo "SNR vs Latency data and plot generated successfully."
