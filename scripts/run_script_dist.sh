#!/bin/bash
# usage: run script on all nodes

source "./load_eval_settings.sh"

if [ "$#" -lt "1" ]; then
	echo "Usage: $0 <script>.sh <args>" >&2
    exit 1
fi

script=$1
args=${@:2}

if [ ! -f "$exp_script_dir/$script" ]; then
    echo "script $script does not exist in directory $exp_script_dir"
    exit 1
fi

for idx in $(seq 1 $((num_nodes-1))); do
    node_ip=${node_ip_list[$idx]}
    
    # run script with bash
    echo ssh -n $user_name@$node_ip "cd $exp_script_dir && bash -i $script $args"
    ssh -n $user_name@$node_ip "cd $exp_script_dir && bash -i $script $args"
done
