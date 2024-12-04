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


def work(combination):
    commands = []
    arguments = []
    standard = ""
    gi = 0
    width = 0
    for i, value in enumerate(combination):
        if options_names[i] == "standard":
            standard = value
        if options_names[i] == "RngRun":
            arguments.append("--RngRun=%s" % value)
            arguments.append("--seed=%s" % value)
        elif options_names[i] == "channelWidth":
            width = value
            arguments.append("--serverChannelWidth=%s" % value)
            arguments.append("--clientChannelWidth=%s" % value)
        elif options_names[i] == "guardInterval":
            gi = value
            arguments.append("--serverShortGuardInterval=%s" % value)
            arguments.append("--clientShortGuardInterval=%s" % value)
        elif options_names[i] == "SS":
            arguments.append("--serverNss=%s" % value)
            arguments.append("--clientNss=%s" % value)
        elif options_names[i] == "test":
            arguments.append("--test=%s" % value)

        else:
            arguments.append("--%s=%s" % (options_names[i], value))

        if len(arguments) == 14:
            managers = config.get("wifiManager", [])
            for manager in managers:
                arguments.append(f"--wifiManager={manager}")
                commands.append(('./ns3 run --no-build "figure-7 %s"' % (" ".join(arguments))))
                arguments.pop()

    for command in commands:
        os.system(command)


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

for combination in combinations:
    work(combination)
