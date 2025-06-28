#!/bin/bash
# # usage: reset and restart OpenEC

source "./load_eval_settings.sh"

# stop OEC
cd ${oec_script_dir}
python2.7 stop.py

# clear metadata
cd ${proj_dir}
rm -f coor_output
rm -f entryStore
rm -f poolStore

# clear logs
rm -f coor_output
rm -f hs_err*
for idx in $(seq 0 $((num_nodes-1))); do
    node_ip=${node_ip_list[$idx]}
    
    ssh -n $user_name@$node_ip "bash -c 'cd ${proj_dir}; rm -f agent_output hs_err*'"
    ssh -n $user_name@$node_ip "echo $user_passwd | sudo -S service redis_6379 restart"
done

# restart HDFS
cd ${exp_script_dir}
bash restart_hdfs.sh

sleep 2

# restart OpenEC
cd ${oec_script_dir}
python2.7 start.py
