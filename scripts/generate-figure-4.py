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

def create_filename(tag, filename, run):
    return "OUTPUT/%s_%s_run=%s.csv" % (tag, filename, run)


def work(combination):
    arguments = []
    filename_pieces = [script_name, str(repeats)]
    for i, value in enumerate(combination):
        arguments.append("--%s=%s" % (options_names[i], value))
        filename_pieces.append("%s=%s" % (options_names[i], value))

    filename = "_".join(filename_pieces).replace("/", "-")
    cli_string = " ".join(arguments)
    for x in range(0,repeats):
        command_line = ('./ns3 run --no-build \"%s %s --RngRun=%s\"' % (script_name, cli_string, x))
        os.system(command_line)

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

ignored_keys = set({"version", "script", "repeats"})
options_names = []
options_values = []

for key, value in config.items():
    if key in ignored_keys:
        continue
    options_names.append(key)
    options_values.append(value)

repeats = config["repeats"]
script_name = config["script"]

combinations = list(itertools.product(*options_values))

for combination in combinations:
    work(combination)

