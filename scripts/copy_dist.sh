#!/bin/bash
# usage: copy file/dir to all data nodes

source "./load_eval_settings.sh"

if [ "$#" != "2" ]; then
	echo "Usage: $0 src_file/src_dir dst_dir" >&2
    exit 1
fi

src_file=$1
dst_dir=$2

# copy file/dir
for idx in $(seq 1 $((num_nodes-1))); do
    node_ip=${node_ip_list[$idx]}
    
    echo rsync -av --recursive $src_file $user_name@$node_ip:$dst_dir
    rsync -av --recursive $src_file $user_name@$node_ip:$dst_dir
done
