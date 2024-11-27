Steps to reproduce plots found in:
Verification of ns-3 Wi-Fi Rate Adaptation Models on AWGN Channels
authored by Juan Leon, Tom Henderson, Sumit Roy
(published in Proceedings of 2023 Workshop on ns-3)
==================================================================

This file overwrites the usual README.md file in ns-3, with instructions
on how to reproduce Figures 4, 5, and 7 of the WNS3 2023 submission.
It also provides a program to test all combinations of parameters for
the three rate control algorithms as described in Section 3.1 Basic Operation.

The code is based on ns-3.37 release with new C++ programs found
in the scratch directory, and small modifications to ns-3's gnuplot
and wifi modules.  No change to the wifi module logic has been
made; the only change was to introduce a new trace source,
WifiRemoteStationManager::TxVectorChange, to facilitate the plots.

## Table of Contents:

1) [File Organization](#file-organization)
2) [Initial Setup](#initial-setup)
3) [Steps to reproduce plots found in Figures 4, 5 and 7](#Figures-4-5-7)

## File Organization

This repository is based on ns-3.37 with the following changes and additions.

1) This README.md file has been overwritten
2) Four C++ programs in the scratch/ directory
3) Python and YAML files in the scripts/ directory
4) Small patches to the wifi and stats modules
5) Top-level shell scripts to reproduce figures

## Initial Setup

Prerequisites:  You must have the following Python modules installed:

  matplotlib, pandas, numpy, collections, statistics, yaml

and some other modules that are typically built-in but may not be in all
Python distributions.  See the import statements in the Python scripts in
the scripts/ directory.

1) Configure ns-3 in optimized mode for fastest execution

```shell
./ns3 configure -d optimized --enable-examples
```

2) Run shell scripts to regenerate figures from the paper.

There are three figures (4, 5, and 7) in the paper that show simulation
data.  Additionally, the paper notes that the rate controls were
tested by stepping through a range of received signal strength values
and by checking all valid combinatorial parameter sets for each rate control,
and confirming that they converged to an appropriate MCS for the given SNR.
Scripts to reproduce these tests are also provided.

First, give execution permissions to shell scripts, if not already done.

```shell
sudo chmod +x generate-figure-4.sh generate-figure-5.sh generate-figure-7.sh run-raa-combinations.sh
```

Then, run any or all of the above scripts depending on the results that
need to be reproduced, such as:

```shell
./generate-figure-4.sh
./generate-figure-5.sh
./generate-figure-7.sh
./run-raa-combinations.sh
```
Note that the simulations to generate all of the data for Figure 7 and
to run all of the combinations take a long time to run (possibly days,
depending on the CPU).  The scripts do not parallelize across multiple
CPU cores.

3) Output files:

Output files produced:

generate-figure-4.sh produces:
- figure-4-output.txt
- figure-4.png

generate-figure-5.sh produces:
- figure
- figure-5-Ideal.eps
- figure-5-Ideal.plt
- figure-5-MinstrelHt.eps
- figure-5-MinstrelHt.plt
- figure-5-ThompsonSampling.eps
- figure-5-ThompsonSampling.plt

generate-figure-7.sh produces:
- figure-7a.csv
- figure-7a-noconvergence.csv
- figure-7a.png
- figure-7b.csv
- figure-7b-noconvergence.csv
- figure-7b.png
- figure-7c.csv
- figure-7c-noconvergence.csv
- figure-7c.png
- figure-7d.csv
- figure-7d-noconvergence.csv
- figure-7d.png

run-raa-combinations.sh
- all-combinations.txt

4) Errata

The scenario details in Figure 5a do not exactly match the simulations in 
Figures 5b,c,d.  The starting SNR was 32 dB, not 31 dB, and the SNR decrease was
20 dB in Figure 5b for Ideal but 25 dB for Figures 5c and 5d.  These changes
from parameters described in Figure 5a were made to better illustrate
convergence behavior, but the small differences were not removed from the plots
and figures before camera-ready submission.
