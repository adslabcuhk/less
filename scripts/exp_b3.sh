#!/bin/bash
# usage: Exp#B3: Encoding throughput

set -e

source "./load_eval_settings.sh"

echo "Exp#B3: Encoding throughput"

# install the ec-library
echo "Installing the ec-library"
cd $proj_dir/src/ec-library
echo $user_passwd | sudo -S apt-get install libtool autoconf
mkdir build && cd build
cmake .. && make

# install dependencies
echo $user_passwd | sudo -S apt-get install python3-pip
pip3 install pathlib numpy scipy

# run experiment
cd $exp_script_dir
python3 exp_b3.py
