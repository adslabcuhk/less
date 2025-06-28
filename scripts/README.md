# List of Scripts

## List of evaluation scripts

Below are the scripts that needs to run or modify:

| Name | Description |
| ------ | ------ |
| settings.ini | Evaluation settings file |
| node_list.txt | List of Node IPs |
| setup.sh | Set up the testbed for evaluation |
| exp_a1.py | Script for Exp#A1: Single-block repair |
| exp_a2.py | Script for Exp#A2: Multi-block repair |
| exp_b1.sh | Script for Exp#B1: Single-block repair |
| exp_b2.sh | Script for Exp#B2: Full-node recovery |
| exp_b3.sh | Script for Exp#B3: Encoding throughput |
| exp_b4.sh | Script for Exp#B4: Impact of network bandwidth |
| exp_b5.sh | Script for Exp#B5: Impact of packet size |

## List of helper scripts

Below are the helper scripts for evaluation:

| Name | Description |
| ------ | ------ |
| load_eval_settings.sh | Load evaluation settings |
| download_deps.sh | Download dependencies |
| install_deps.sh | Install dependencies |
| set_ssh_dist.sh | (All nodes) Set up password-less connections |
| test_login_dist.sh | (All nodes) Test user login |
| compile_hadoop.sh | Compile Hadoop |
| install_oec.sh | Install OpenEC |
| common.py | Common functions for Python |
| set_bw.sh | Set bandwidth for the testbed |
| clear_bw.sh | Clear bandwidth settings on the testbed |
| benchmark_disk.sh | Benchmark disk performance |
| benchmark_bw_dist.sh | (All nodes) Benchmark bandwidth in the cluster |
| copy_dist.sh | (All nodes but self) Copy files to all nodes but itself |
| run_script_dist.sh | (All nodes but self) Run a script on all nodes but itself |
| update_config_dist.sh | (All nodes) Update OpenEC and HDFS configurations on all nodes
| update_ip.sh | Update IP addresses in the configuration files |
| gen_oec_config.py | Generate OpenEC configuration file |
| restart_hdfs.sh | Restart HDFS |
| restart_oec.sh | Restart OpenEC |
| exp_b3.py | Helper script for Exp#B3 |
| get_full_node_recovery_time.sh | Get full node recovery time |
| code_test_list.txt | List of Erasure codes to test |