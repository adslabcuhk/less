#!/bin/bash
# update configs: ip, block size and packet size

source "./load_eval_settings.sh"

if [ "$#" != "3" ]; then
	echo "Usage: $0 block_size (Byte) packet_size (Byte) bandwidth (Kbps)" >&2
    exit 1
fi

block_size=$1
packet_size=$2
bandwidth=$3

# update settings.ini
sed -i "s/^\(block_size_byte *= *\).*\$/\1${block_size}/" $INI_FILE
sed -i "s/^\(packet_size_byte *= *\).*\$/\1${packet_size}/" $INI_FILE
sed -i "s/^\(bandwidth_kbps *= *\).*\$/\1${bandwidth}/" $INI_FILE

# update OpenEC and HDFS configs locally
python3 gen_oec_config.py -f $INI_FILE

cp -r $proj_dir/hdfs3.3.4-integration/conf/* ${hadoop_home_dir}/etc/hadoop

# update node list
cp node_list.txt ${hadoop_home_dir}/etc/hadoop/workers
sed -i '1d' ${hadoop_home_dir}/etc/hadoop/workers # remove the NameNode

# update NameNode IP
# change to hdfs://<namenode_ip>:9000
# name node ip: the first line of node_list.txt
namenode_ip=$(head -1 node_list.txt | sed "s/ *//g")
sed -i "s%\(<property><name>fs.defaultFS</name><value>\)[^<]*%\1hdfs://${namenode_ip}:9000%" ${hadoop_home_dir}/etc/hadoop/core-site.xml
sed -i "s%\(<property><name>oec.controller.addr</name><value>\)[^<]*%\1${namenode_ip}%" ${hadoop_home_dir}/etc/hadoop/hdfs-site.xml
bash update_ip.sh $namenode_ip

# block size
block_size_MiB=`expr $block_size / 1024 / 1024`
sed -i "s%\(</ecid><base>\)[^<]*%\1${block_size_MiB}%" ${conf_dir}/${conf_filename}
sed -i "s%\(<property><name>dfs.blocksize</name><value>\)[^<]*%\1${block_size}%" ${hadoop_home_dir}/etc/hadoop/hdfs-site.xml

# packet size
sed -i "s%\(<attribute><name>packet.size</name><value>\)[^<]*%\1${packet_size}%" ${conf_dir}/${conf_filename}
sed -i "s%\(<property><name>oec.pktsize</name><value>\)[^<]*%\1${packet_size}%" ${hadoop_home_dir}/etc/hadoop/hdfs-site.xml

# others
hdfs_tmp_dir=/home/${user_name}/hadoop-3.3.4
sed -i "s%\(<property><name>hadoop.tmp.dir</name><value>\)[^<]*%\1${hdfs_tmp_dir}%" ${hadoop_home_dir}/etc/hadoop/core-site.xml

# distribute the OpenEC and HDFS configs to all nodes but self
bash copy_dist.sh ${conf_dir}/${conf_filename} ${conf_dir}
bash copy_dist.sh ${hadoop_home_dir}/etc/hadoop ${hadoop_home_dir}/etc

# Update local IPs on all data nodes
for idx in $(seq 1 $((num_nodes-1))); do
    node_ip=${node_ip_list[$idx]}
    
    # run script with bash
    ssh -n $user_name@$node_ip "cd $exp_script_dir && bash -i update_ip.sh $node_ip"
done
