#!/bin/bash
# usage: test ssh connection on each node

source "./load_eval_settings.sh"

for idx in $(seq 0 $((num_nodes-1))); do
    node_ip=${node_ip_list[$idx]}
    
    expect << EOF
    
    set timeout 3
    spawn ssh $user_name@$node_ip "echo success"
    expect {
        "*yes/no" { send "yes\n"; exp_continue }
        "*password" { send "$user_passwd\n" }
    }
     
EOF
done
