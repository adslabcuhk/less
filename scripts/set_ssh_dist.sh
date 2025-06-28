#!/bin/bash
# setup ssh password-less connection on each node

source "./load_eval_settings.sh"

echo "Setting up SSH password-less connection on each node..."

if [ -z "$user_private_key" ]; then
    echo "User private key is not set. Using password authentication."
    if [ ! -f ~/.ssh/id_rsa ]; then
        ssh-keygen -t rsa -P "" -f /home/$user_name/.ssh/id_rsa
    else
        cp $user_public_key ~/.ssh/id_rsa.pub
        cp $user_private_key ~/.ssh/id_rsa
    fi
fi

for idx in $(seq 0 $((num_nodes-1))); do
    node_ip=${node_ip_list[$idx]}
    
    ssh-keyscan -H ${node_ip} >> ~/.ssh/known_hosts
    ssh-copy-id -f -i ~/.ssh/id_rsa.pub ${user_name}@${node_ip}
done
