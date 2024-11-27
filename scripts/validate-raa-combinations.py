#!/usr/bin/env python3

# Copyright 2019 Remy Grunblatt
# Copyright 2023 University of Washington (adapted from original)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation;
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Original script provided courtesy of Remy Grunblatt
# See: https://theses.hal.science/INSA-LYON-THESES/tel-03126953v1

import csv
import itertools
import random
import subprocess
import os
import sys
from multiprocessing.pool import ThreadPool
import shutil
from pathlib import Path
import glob
import argparse
import yaml
from collections import defaultdict

commands = []

def generate_command(combination):
    command = [] 
    arguments = []
    standard = ""
    gi = 0
    width = 0

    for i, value in enumerate(combination):
        if(options_names[i] == "standard"):
            standard = value
        if(options_names[i] == "RngRun"):
            arguments.append("--RngRun=%s" % (value))
            arguments.append("--seed=%s" % value)
        elif(options_names[i] == "channelWidth"):
            width = value
            arguments.append("--serverChannelWidth=%s" % (value))
            arguments.append("--clientChannelWidth=%s" % (value))
        elif(options_names[i] == "guardInterval"):
            gi = value
            arguments.append("--serverShortGuardInterval=%s" % (value))
            arguments.append("--clientShortGuardInterval=%s" % (value))
        elif(options_names[i] == "SS"):
            arguments.append("--serverNss=%s" % (value))
            arguments.append("--clientNss=%s" % (value))
        else:
            arguments.append("--%s=%s" % (options_names[i], value))
 
        # Filter out invalid combinations
        if((standard == "802.11n-2.4GHz" or standard == "802.11n-5GHz" or standard == "802.11ax-2.4GHz") and (width == 80 or width == 160)):
            arguments = []
            continue
        if((standard == "802.11n-2.4GHz" or standard == "802.11n-5GHz" or standard == "802.11ac") and (gi==1600 or gi==3200)):
            arguments = []
            continue
        if(standard == "802.11ax-2.4GHz" or standard == "802.11ax-5GHz" or standard == "802.11ax-6GHz") and (gi == 400):
            arguments = []
            continue

    if (len(arguments) > 0):
    	command.append(('./ns3 run --no-build \"validate-raa-combinations %s\"' % ((" ".join(arguments)))))

    return command

my_env = os.environ.copy()

parser = argparse.ArgumentParser()
parser.add_argument("config", type=str, help="The name of the yaml config file used for simulation")
args = parser.parse_args()

with open(args.config, 'r') as stream:
    try:
        config = yaml.safe_load(stream)
    except yaml.YAMLError as exc:
        print(exc)

PWD = os.path.dirname(os.path.abspath(__file__))

random.seed("RG,IGL,OS")

options_names = []
options_values = []

for key, value in config.items():
    options_names.append(key)
    options_values.append(value)

combinations = list(itertools.product(*options_values))

random.shuffle(combinations)
combinations.sort()

i = 1
for combination in combinations:
    command = generate_command(combination)
    if (len(command) > 0):
        commands.append(command)
    i = i + 1
    
i = 1
for command in commands:
    command_string = ''.join(command)
    print("Running %d of %d simulations: %s" % (i, len(commands), command_string))
    os.system(command_string)
    i = i + 1
