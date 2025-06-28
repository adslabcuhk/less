#!/bin/bash
# usage: reset and restart HDFS

source "./load_eval_settings.sh"

stop-dfs.sh

# namenode
rm -rf $hadoop_home_dir/tmp
rm -rf $hadoop_home_dir/logs

# execute command
for idx in $(seq 0 $((num_nodes-1))); do
    node_ip=${node_ip_list[$idx]}
     
    ssh -n $user_name@$node_ip "rm -rf $hadoop_home_dir/tmp"
    ssh -n $user_name@$node_ip "rm -rf $hadoop_home_dir/logs"
    ssh -n $user_name@$node_ip "rm -rf $hadoop_home_dir/dfs"
done

hdfs namenode -format -force && start-dfs.sh
