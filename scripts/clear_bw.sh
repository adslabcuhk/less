#!/bin/bash
# usage: clear bandwidth setting on a node
source "./load_eval_settings.sh"

wondershaper_dir=$pkg_dir/wondershaper

node_ip=$(ifconfig | grep $ip_prefix | head -1 | sed "s/ *inet [addr:]*\([^ ]*\).*/\1/")
node_dev=$(ifconfig | grep -B1 $node_ip | grep -o "^\w*")

cd $wondershaper_dir && echo $user_passwd | sudo -S ./wondershaper -c -a $node_dev

echo clear bandwidth setting [ $cur_ip $node_dev ]