#!/bin/bash
# usage: load evaluation settings to bash variables

home_dir=$(echo ~) # home directory
pkg_dir=${home_dir}/packages # package directory
proj_dir=$(dirname "$(pwd)") # project directory
conf_dir=${proj_dir}/conf # OpenEC configuration directory
conf_filename=sysSetting.xml
src_dir=${proj_dir}/src # OpenEC source code directory
oec_script_dir=${proj_dir}/script # OpenEC script directory
exp_script_dir=${proj_dir}/scripts # Experiment script directory
hadoop_home_dir=$(echo $HADOOP_HOME) # Hadoop home directory
hdfs_config_dir=${proj_dir}/hdfs3.3.4-integration/conf # HDFS configuration directory
INI_FILE=${exp_script_dir}/settings.ini # evaluating settings file
node_list_file=${exp_script_dir}/node_list.txt # node list file

# load source 
source $home_dir/.bashrc

# print dir
echo "home_dir:" $home_dir
echo "proj_dir:" $proj_dir
echo "conf_dir:" $conf_dir

# user
user_name=$(grep '^user_name = ' "$INI_FILE" | cut -d '=' -f2)
user_name=$(echo "$user_name" | xargs)
user_passwd=$(grep '^user_passwd = ' "$INI_FILE" | cut -d '=' -f2)
user_passwd=$(echo "$user_passwd" | xargs)
user_public_key=$(grep '^user_public_key = ' "$INI_FILE" | cut -d '=' -f2)
user_public_key=$(echo "$user_public_key" | xargs)
user_private_key=$(grep '^user_private_key = ' "$INI_FILE" | cut -d '=' -f2)
user_private_key=$(echo "$user_private_key" | xargs)

# print user
echo "user_name: "$user_name
echo "user_passwd:" $user_passwd
echo "user_public_key:" $user_public_key
echo "user_private_key:" $user_private_key

# ip_prefix
ip_prefix=$(grep '^ip_prefix = ' "$INI_FILE" | cut -d '=' -f2)
ip_prefix=$(echo "$ip_prefix" | xargs)

# print ip_prefix
echo "ip_prefix:" $ip_prefix

# get node list
node_ip_list=()

while IFS= read -r line
do
    ip=`echo $line | cut -d " " -f 1`
    
    node_ip_list+=($ip)
done < ${node_list_file}

num_nodes=${#node_ip_list[@]}

# print configs
echo "node_list_file:" $node_list_file
echo Number of nodes: $num_nodes

for idx in $(seq 0 $((num_nodes-1))); do
    echo Node $idx IP: ${node_ip_list[$idx]}
done
