# Artifact Evaluation Instructions

<!-- Generate table of contents -->

## Table of Contents
1. [Artifact Claims](#artifact-claims)
2. [Testbed Setup](#testbed-setup)
3. [Evaluation](#evaluation)
   - [Numerical Analysis](#numerical-analysis)
   - [Testbed Experiments](#testbed-experiments)

## Artifact Claims

Our goal is to demonstrate LESS's effectiveness in improving repair
performance as shown in our FAST 2026 paper.  For numerical analysis (i.e.,
Exp\#A1 and Exp\#A2), we expect the results to match those in our paper. For
testbed experiments (i.e., Exp\#B1 to Exp\#B5), we expect that LESS reduces
the single-block repair and full-node recovery times compared to the baseline
erasure codes. However, the testbed experiment results may vary from those in
our paper due to different factors, such as cluster sizes, machine
specifications, operating systems, and software packages.

## Hardware Dependencies

We require and assume 15 machines to form a cluster for running the
experiments with our prototype and evaluation scripts. These machines need to
be connected via a 10 Gbps network, such that they are reachable from each
other. For each machine, we recommend a quad-core CPU, 16 GiB of memory, a
7200 RPM SATA HDD and above. We use the default erasure coding parameters (n,
k) = (14, 10). In particular, we use one machine to run the HDFS NameNode and
the other machines to run the HDFS DataNodes.

## Testbed Setup

**Time estimation**: ~ 10 human-minutes + ~ 300 compute minutes

**Assumptions**: To simplify the testbed setup, we assume all nodes share the
same following default configurations:

```
Operating System: Ubuntu 22.04 LTS
Path to artifact: "/home/${user_name}/less"
```


We provide scripts for automatic testbed setup. The scripts in ```scripts/```
are tested on Ubuntu 22.04 LTS, with a ```README.md``` file describing their
usages. Note that the actual running times of the scripts depend on the
cluster sizes, machine specifications, operating systems, and software
packages.

For each node in the cluster, please follow the steps below:

Step 1: Set up the node with ```user_name/user_passwd```. This ensures
automatic testbed setup with the scripts later.  For Chameleon cloud users,
the instances created with CC-* series system image have the default user name
"cc" and no password (but with SSH keys).

Step 2: Modify ```scripts/settings.ini``` to change the evaluation settings.
There are a few settings you may need to change:

```
[Experiment]
num_runs = 1 # the number of runs in experiments

[Cluster]
user_name = cc # the username of the root user (For Chameleon cloud users, please keep this as "cc".)
user_passwd = # the password of the root user (For Chameleon cloud users, please keep this empty.)
user_public_key = /home/cc/.ssh/id_rsa.pub # Path to cluster SSH public key
user_private_key = /home/cc/.ssh/id_rsa # Path to cluster SSH private key (Only used for Chameleon Cloud instance created with CC-* series system image; make sure this private key is used for ssh to the instance). For nodes without private_key, please keep this empty. We assume all the nodes have the same path.
ip_prefix = 192.168.10 # the IP prefix of the cluster nodes
```

Step 3: Modify ```scripts/node_list.txt``` to include the IP addresses of all
nodes in the cluster. Each line should only contain one IP address, and the
file should not contain any empty lines or comments.  The ```ip_prefix```
should be `192.168.10` in ```settings.ini```. See the example below:

```
192.168.10.1
192.168.10.2
...
192.168.10.15
```

We assume **the first machine** (with the first IP address) serves as the HDFS
NameNode, and the other 14 machines serve as HDFS DataNodes.

On the NameNode, run ```scripts/setup.sh``` to install all the software
dependencies, set up the environment variables on all nodes, and set up mutual
password-less SSH connection between all nodes.

```
cd scripts
bash setup.sh
source ~/.bashrc
```

To verify the installation is successful, please check the following items:

* Check if the terminal outputs any essential error messages.
* Test SSH password-less login: run ```bash test_login_dist.sh``` should return "success" for all nodes.
* Redis: On each node, run ```redis-cli``` should enter the Redis CLI without any error.
* Hiredis: On each node, run ```find /usr /usr/local -name hiredis.h
  2>/dev/null``` should return the path to the hiredis header file, e.g.,
  ```/usr/local/include/hiredis/hiredis.h```.
* GF-Complete: On each node, run ```find /usr /usr/local -name gf_complete.h
  2>/dev/null``` should return the path to the gf_complete header file, e.g.,
  ```/usr/local/include/gf_complete.h```.
* ISA-L: On each node, run ```find /usr /usr/local -name isa-l.h
  2>/dev/null``` should return the path to the isa-l header file, e.g.,
  ```/usr/include/isa-l.h```.
* JAVA: On each node, run ```cd $JAVA_HOME``` should enters the Java home
  directory ```/usr/lib/jvm/java-8-openjdk-amd64```.
* Maven: On each node, run command ```mvn -v``` should show the Maven version.
* Hadoop: the building process could take a long while (~4 hours), and it
  should not report any error. On each node, run
  command ```hadoop version``` should show the Hadoop version as 3.3.4.
* OpenEC: On each node, the compilation should generate the binaries without
  any error. After the installation, check whether ```OECCoordinator``` and
  ```OECAgent``` exist in the project directory.


## Evaluation

Before running any experiment, please make sure that the testbed setup has
been completed.  Then enter the ```scripts/``` directory:
```
cd scripts
```


The default OpenEC configuration file is in ```conf/sysSetting.xml```.
It includes configurations for different (n, k) = (14, 10) erasure codes
evaluated in our paper:

| Code | ClassName | Parameters *(n,k)* | Sub-packetization (alpha) |
| ------ | ------ | ------ | ------ |
| RS codes | RSCONV | (14,10) | 1 |
| Clay codes | Clay | (14,10) | 256 |
| HashTag | HTEC | (14,10) | 2, 3, 4 |
| Hitchhiker (Hitchhiker-XOR+ version) | HHXORPlus | (14,10) | 2 |
| Elastic Transformation (base code: RS codes) | ETRSConv | (14,10) | 2, 3, 4 |
| LESS | LESS | (14,10) | 2, 3, 4 |

**NOTE**: For Chameleon cloud users, if you are using the CC-* series system
image, there are [firewall
settings](https://chameleoncloud.readthedocs.io/en/latest/technical/networks/networks_basic.html#firewall)
that need to be configured to allow connections for different applications. In
particular, please make sure the ports are open to allow connections for HDFS
and OpenEC. On each node, please run below command to open the ports for the IPs ```192.168.10.0/24```:

```
sudo firewall-cmd --zone=trusted --add-source=192.168.10.0/24 --permanent
sudo firewall-cmd --reload
```

### Numerical Analysis (for a quick start)

#### Exp#A1 Single-block Repair

**Time estimation**: ~ 1 human-minutes + ~ 2 compute minutes

This experiment evaluates single-block repair performance of different
erasure codes. The results includes single-block repair I/O and I/O seeks.

Run ```exp_a1.py``` to generate the results for different codes:
```
python3 exp_a1.py
```

The repair I/O and I/O seeks will be printed on the terminal, which exactly
matches the results in Tables 2 and 3. See the results for LESS (14,10,4) below:

```
Code: LESS (14, 10, 4)
Repair I/O (bandwidth): avg: 4.642857142857143, min: 4.0, max: 4.75
I/O seeks: avg: 13.0, min: 13, max: 13
```

#### Exp#A2 Multi-block Repair

**Time estimation**: ~ 1 human-minutes + ~ 1 compute minutes

This experiment evaluates multi-block repair performance of LESS and RS codes.
The results includes two-block repair I/O, I/O seeks, and the improved repair
ratio.  Note that there is no need to run experiment for RS codes, as the
repair I/O and I/O seeks are both equal to k.

Run ```exp_a2.py``` to generate the results for LESS:
```
python3 exp_a2.py
```

The repair I/O, I/O seeks, and improved repair ratio will be printed on
the terminal, which exactly matches the results in Figure 3. See the results
for LESS (14,10,2) below:
```
LESS (14, 10, 2), 2-blocks repair
Repair I/O (bandwidth): avg: 9.252747252747254
I/O seeks: avg: 10.571428571428571
Improved repair (with reduced repair I/O and I/O seeks) ratio: 0.2857142857142857
```
LESS reduces the average repair I/O of RS codes by (10 - 9.252747252747254) /
10 = 7.4%, and improves for 28.6% of cases.


### Testbed Experiments

For a quick start, please refer to [Exp#B3: Encoding
Throughput](#expb3-encoding-throughput), which runs on a single node. The
other experiments run on a cluster of nodes.

Note that the actual repair time depends on the hardware and software
configurations of the testbed, and could be different from the paper's
results.

#### Exp#B1 Single-block Repair

**Time estimation**: ~ 1 human-minutes + ~ 120 compute minutes

This experiment evaluates single-block repair time of different erasure codes.
We measure single-block repair time (in seconds) in OpenEC, with the default
network bandwidth 1Gbps, block size 64MiB, and packet size 256KiB.

Run ```exp_b1.sh``` to generate the results:
```
bash exp_b1.sh
```

The repair time will be printed on the terminal. See the results for LESS
(14,10,4) below:

```
Code: LESS (14, 10, 4)
Network bandwidth: 1Gbps, block size: 64MiB, packet size: 256KiB

Single-block repair time (sec): avg: 2.7268835714285715, lower: 2.297546428571428, upper: 2.962175714285715
```

which shows that the single-block repair time for LESS (14,10,4) is around 2.7
seconds. See also the intermediate results showing the repair time of the 1st
block of a LESS (14,10,4) stripe below:

```
Execute Command: ssh cc@192.168.0.95 "cd /home/cc/less && ./OECClient read /Stripe_0_LESS_14_10_4_0 Stripe_0_LESS_14_10_4_0"
OECInputStream::readWorker.duration: 2743.93
OECInputStream::output2file.time = 2748.64
read.overall.duration: 2749.87
results for code LESS_14_10_4 block 0: 2749.87
save LESS_14_10_4 result in /home/cc/less/eval_results/single/LESS_14_10_4/bw1Gbps_blk64MiB_pkt256KiB/block_0.txt
Finished evaluation for code LESS_14_10_4, block 1
```

which shows that the single-block repair time for LESS (14,10,4) is around 2.7 seconds.

#### Exp#B2 Full-node Recovery

**Time estimation**: ~ 1 human-minutes + ~ 120 compute minutes

This experiment evaluates full-node recovery time of different erasure codes.
We measure full-node recovery time (in seconds) in OpenEC, with the default
configurations as Exp#B1.

Run ```exp_b2.sh``` to generate the results:
```
bash exp_b2.sh
```

The repair time will be printed on the terminal. See the results for LESS (14,10,4) below:

```
Code: LESS (14, 10, 4)
Network bandwidth: 1Gbps, block size: 64MiB, packet size: 256KiB

Full-node recovery time (sec): avg: 61.49134506, lower: 60.58655505, upper: 62.39613508
```

which shows that the average full-node recovery time for LESS (14,10,4) is
around 61.5 seconds.

#### Exp#B3 Encoding Throughput

**Time estimation**: ~ 1 human-minutes + ~ 5 compute minutes

This experiment evaluates encoding throughput (MiB/s) of LESS and RS codes. We
run a standalone program to encode a single (14,10) stripe in memory, with
packet sizes varied from 128 KiB t o 1024 KiB.

Run ```exp_b3.sh``` to generate the results for LESS and RS codes:
```
bash exp_b3.sh
```

The encoding throughputs will be printed on the terminal.  See the results for
LESS (14,10) with 256 KiB packet size below:

```
LESS (14, 10, 4), packet size (bytes): 262144, Encoding throughput (MiB/s): avg: 1643.149958, lower: 1611.547963, upper: 1675.171492
```


#### Exp#B4 Impact of Network Bandwidth

**Time estimation**: ~ 1 human-minutes + ~ 120 compute minutes

This experiment evaluates the impact of network bandwidth for single-block
repair time of LESS, RS codes and Clay codes. We measure single-block repair
time (in seconds) in OpenEC with different network bandwidth (1, 2, 5, 10
Gbps).

Run ```exp_b4.sh``` to generate the results:
```
bash exp_b4.sh
```

The repair time will be printed on the terminal. See the results for LESS
(14,10,4) in a 10Gbps network below:

```
Code: LESS (14, 10, 4)
Network bandwidth: 10Gbps, block size: 64MiB, packet size: 256KiB

Single-block repair time (sec): avg: 0.5253683786, lower: 0.5181347995, upper: 0.5326019577
```

which shows that the single-block repair time for LESS (14,10,4) in a 10Gbps
network is around 0.53 seconds.

#### Exp#B5 Impact of Packet Size

**Time estimation**: ~ 1 human-minutes + ~ 120 compute minutes

This experiment evaluates the impact of packet size for single-block repair
time of LESS, RS codes and Clay codes. We measure single-block repair time (in
seconds) in OpenEC with different packet sizes (128, 256, 512, 1024 KiB).

Run ```exp_b5.sh``` to generate the results for LESS:
```
bash exp_b5.sh
```

The repair time will be printed on the terminal. See the results for LESS
(14,10,4) with 128KiB packet size below:

```
Code: LESS (14, 10, 4)
Network bandwidth: 1Gbps, block size: 64MiB, packet size: 128KiB

Single-block repair time (sec): avg: 2.755653143, lower: 2.74564861, upper: 2.765657676
```

which shows that the single-block repair time for LESS (14,10,4) with 128KiB packet size is around 2.76 seconds.