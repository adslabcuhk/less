#!/bin/bash

set -e

source "./load_eval_settings.sh"

find . -type f -name "*.sh" -exec chmod +x {} \;
find . -type f -name "*.py" -exec chmod +x {} \;

# download dependencies
bash download_deps.sh

# set up ssh password-less connection on each node
bash set_ssh_dist.sh

# copy dependencies to all DataNodes
bash copy_dist.sh $proj_dir $home_dir
bash copy_dist.sh $pkg_dir $home_dir

# install dependencies on each node
for idx in $(seq 0 $((num_nodes-1))); do
    node_ip=${node_ip_list[$idx]}

    echo "start to install dependencies on $node_ip"
    echo

    echo $user_passwd | ssh $user_name@$node_ip "cd $exp_script_dir && bash install_deps.sh"

    echo "finish installing dependencies on $node_ip"
    echo
done

# compile Hadoop 3 (this may take a long while)
bash compile_hadoop.sh
 
# copy Hadoop to all DataNodes
source ~/.bashrc
bash copy_dist.sh $HADOOP_HOME $home_dir

# compile OpenEC on each node
for idx in $(seq 0 $((num_nodes-1))); do
    node_ip=${node_ip_list[$idx]}

    echo "start to install OpenEC on $node_ip"
    echo

    echo $user_passwd | ssh $user_name@$node_ip "cd $exp_script_dir && bash install_oec.sh"

    echo "finish installing OpenEC on $node_ip"
done

echo "Finished testbed setup!"
