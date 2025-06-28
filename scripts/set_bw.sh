#!/bin/bash
# usage: set up bandwidth limit on a node

source "./load_eval_settings.sh"

if [ "$#" != "1" ]; then
	echo "Usage: $0 bw (Kbps)" >&2
    exit 1
fi

bw_Kbps=$1

wondershaper_dir=$pkg_dir/wondershaper

node_ip=$(ifconfig | grep $ip_prefix | head -1 | sed "s/ *inet [addr:]*\([^ ]*\).*/\1/")
node_dev=$(ifconfig | grep -B1 $node_ip | grep -o "^\w*")

cd $wondershaper_dir && echo $user_passwd | sudo -S ./wondershaper -a $node_dev -u $bw_Kbps -d $bw_Kbps
cd -

echo enable bandwidth setting: $node_ip $node_dev $bw_Kbps Kbps
