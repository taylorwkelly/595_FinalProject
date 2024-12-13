Final Project for EE595A: Taylor Kelly and Rami Ayari
==================================================================

This project focused on simulating and testing various different RAA algorithms
found within NS3. As a base, it used the Verification of ns-3 Wi-Fi Rate Adaptation Models on AWGN Channels
authored by Juan Leon, Tom Henderson, Sumit Roy paper, and its associated repository,
to implement CARA, AARF, and Minstrel and test them in various network conditions.
This is the codebase where the graphs and scripts to generate those graphs are from

## File Organization

The main changes that were made were the addition of various scripts and files that modify the
functionality of what data was inputted. The new YAML files contain the various parameters,
with various version of complexity. There are also python files which generate the graphs found from our
project.

## Files to run
The following new scripts were added in order to generate the graphs.
Each has a type of graph that it generates:


```shell
./all_data.sh
./path_model_testing.sh
./packet_loss.sh
./distance_throughput.sh
```

Specifically for generating the plots for seeing the change in MCS, there are significant changes. Due to the older wifi versions and algorithms
used, the MCS no longer applies, and is instead replased with a specific OFDM protocol. This is to allow the usage of the previous code without
significant overhauls, and to still display the change in throughput at a significant SNR decrease.
To generate the plots for each algorithm, simply run:

```shell
./generate-figure-5.sh
```
and the .eps files can be converted into graphs.

Each of the scripts generates a different graph, with the all_data script generating the data that can be used to make the graphs.
These do take a while to run, so in order to run them once, running the all_data script, then changing the bash scripts to use it as
input data can be substituted to get graphs.

Of note, in this repository, we also have the graphs that were generated for the project for ease of access, as well as various txt files
that represent the data used.
