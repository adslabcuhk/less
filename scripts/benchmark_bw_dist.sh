#!/bin/bash
# usage: benchmark bandwidth between current node and other nodes
# Current node will run iperf server, and the other nodes will run iperf clients

source "./load_eval_settings.sh"

if [ "$#" != "1" ]; then
	echo "Usage: $0 node_ip" >&2
    exit 1
fi

cur_node_ip=$1

iperf -s &

for idx in $(seq 0 $((num_nodes-1))); do
    node_ip=${node_ip_list[$idx]}

    echo Current Node: ${cur_node_ip} Destination Node: ${node_ip}
    echo $user_passwd | ssh $user_name@$node_ip iperf -c ${cur_node_ip}
done

killall iperf
