#!/bin/bash

source "./load_eval_settings.sh"

if [ "$#" == "0" ]; then
    node_ip=$(ifconfig | grep $ip_prefix | head -1 | sed "s/ *inet [addr:]*\([^ ]*\).*/\1/")
elif [ "$#" == "1" ]; then
	node_ip=$1
fi

conf_filename=sysSetting.xml

sed -i "/<name>local\.addr<\/name>/{n;s/<value>.*<\/value>/<value>${node_ip}<\/value>/;}" ${conf_dir}/${conf_filename}
sed -i "s%\(<property><name>oec.local.addr</name><value>\)[^<]*%\1${node_ip}%" ${hadoop_home_dir}/etc/hadoop/hdfs-site.xml