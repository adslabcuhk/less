#!/bin/bash
# usage: compile openec

source "./load_eval_settings.sh"

cd $proj_dir && cmake . -DFS_TYPE:STRING=HDFS3 && make
