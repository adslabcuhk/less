# LESS Patch to OpenEC


## Overview

We implement LESS atop Hadoop [HDFS
  3.3.4]((https://hadoop.apache.org/docs/r3.3.4/)) with
[OpenEC](https://www.usenix.org/conference/fast19/presentation/li) (USENIX
FAST 2019). OpenEC is a middleware system realizing custom erasure coding
solutions in the form of direct acyclic graphs (named ECDAGs) atop existing
distributed storage systems.  We implement LESS in ECDAGs and integrate to
**OpenEC v1.0.0** ([link](http://adslab.cse.cuhk.edu.hk/software/openec/)).
This repository contains **the patch** to OpenEC.

## Folder Structure

The implementation of erasure codes is in
```openec-patch/src/ec/```.

```
LESS.cc/hh LESS
RSCONV.cc/hh RS codes
HHXORPlus.cc/hh Hitchhiker (Hitchhiker-XOR+ version)
HTEC.cc/hh HashTag
ETRSConv.cc/hh Elastic Transformation (base code: RS codes)
Clay.cc/hh Clay codes
```

The sample configuration file is in ```openec-patch/conf/sysSetting.xml```. We
include the sample configurations for the codes below.


| Code | ClassName | Parameters *(n,k)* | Sub-packetization |
| ------ | ------ | ------ | ------ |
| LESS | LESS | (14,10) | 2, 3, 4 |
| RS codes | RSCONV | (14,10) | 1 |
| Hitchhiker (Hitchhiker-XOR+ version) | HHXORPlus | (14,10) | 2 |
| Elastic Transformation (base code: RS codes) | ETRSConv | (14,10) | 2, 3, 4 |
| HashTag | HTEC | (14,10) | 2, 3, 4 |
| Clay codes | Clay | (14,10) | 256 |


## Deployment

Please follow the deployment steps below:

* [Cluster setup](#cluster-setup)

* [Download HDFS-3.3.4 and OpenEC-v1.0.0](#download-hdfs-334-openec-v100)

* [Add patch to OpenEC-v1.0.0](#add-patch-to-openec-v100)

* [Deploy HDFS-3.3.4 with patched
  OpenEC](#deploy-hdfs-334-with-patched-openec)

* [Start HDFS-3.3.4 with OpenEC](#start-hdfs-334-with-openec)

* [Run Single-block Repair](#run-single-block-repair)

* [Run Full-node Recovery](#run-full-node-recovery)

* [Misc: Run ECDAG Test](#misc-run-ecdag-test)


### Cluster Setup

We setup a cluster of 15 machines (1 for HDFS NameNode, 14 for DataNode). 

| HDFS | OpenEC | Number | IP |
| ------ | ------ | ------ | ------ |
| NameNode | Controller | 1 | 192.168.10.57 |
| DataNode | Agent and Client | 14 | 192.168.10.<47,48,49,50,52,55,56,58,60,62,67,68,69,31> | 

On each machine, we create an account ```less```. Please change the IP
addresses in the configuration files of HDFS (in
```$HADOOP_HOME/etc/hadoop/workers, $HADOOP_HOME/etc/hadoop/core-site.xml, $HADOOP_HOME/etc/hadoop/hdfs-site.xml```) and OpenEC (```conf/sysSetting.xml```) for
your cluster accordingly. Please also refer to OpenEC documentation for more
details.


### Download HDFS-3.3.4, OpenEC-v1.0.0

On the NameNode:

* Download the source code of HDFS-3.3.4 in ```/home/less/hadoop-3.3.4-src```

* Download the source code of OpenEC-v1.0.0 in ```/home/less/openec```

* Download LESS. The patch is in ```/home/less/less/openec-patch```


### Add patch to OpenEC-v1.0.0

Copy the patch into ```openec/```

```
$ cp -r /home/less/less/openec-patch/* /home/less/openec
```


### Deploy HDFS-3.3.4 with patched OpenEC

Please follow the OpenEC documentation for deploying HDFS-3.3.4 and OpenEC in
the cluster.

Notes:

* The sample configuration files for HDFS-3.3.4 are in
```openec-patch/hdfs3.3.4-integration/conf```.

We set the default system configurations as follows:
- HDFS block size: 64 MiB
- OpenEC packet size: 256 KiB

In ```hdfs-site.xml``` of HDFS:

| Parameter | Description | Example |
| ------ | ------ | ------|
| dfs.block.size | The size of a block in bytes. | 67108864 |
| oec.pktsize | The size of a packet in bytes. | 262144 |


In ```conf/sysSetting.xml``` of OpenEC:

| Parameter | Description | Example |
| ------ | ------ | ------ |
| packet.size | The size of a packet in bytes. | 262144 |

The other configurations follow the defaults in OpenEC documentation.

### Compile and deploy HDFS and OpenEC

We follow the default compilation and deployment steps in OpenEC. Please
follow the steps in OpenEC documentation.

### Start HDFS-3.3.4 with OpenEC

* Start HDFS-3.3.4

```
$ hdfs namenode -format
$ start-dfs.sh
```

* Start OpenEC

```
$ cd openec
$ python script/start.py
```


### Run Single-block Repair

Below is an example of single-block repair for LESS with (14, 10), sub-packetization = 4.

#### Write Blocks

On one HDFS DataNode:

- Use OpenEC Client to issue an (online) file write. Assume the filename is
  ```input_640``` and the file size is 640 MiB in *k=10* HDFS blocks. The file
  is encoded into *n=14* HDFS blocks and randomly distributed to *n* storage
  nodes.

```
cd ~/openec
./OECClient write input_640 /test_code LESS_14_10_4 online 640
```

We can check the distribution of blocks with:

```
hdfs fsck / -files -blocks -locations
```

For each block *i* (out of *n*), the filename format in HDFS is
```/test_code_oecobj_<i>```; the physical HDFS block name format is
```blk_<blkid>```.

e.g., the first block (or block 0) is stored in HDFS as
```/test_code_oecobj_0```. We can also get the IP address of DataNode that
stores block 0.

A sample log message copied from ```hdfs fsck``` are shown as below. The IP
address of the DataNode that stores block 0 is ```192.168.10.47```; the
physical block name stored in HDFS is named ```blk_1073741836```.

```
/test_code_oecobj_0 67108864 bytes, replicated: replication=1, 1 block(s):  OK
0. BP-1220575476-192.168.10.57-1672996056729:blk_1073741836_1011 len=67108864 Live_repl=1  [DatanodeInfoWithStorage[192.168.10.47:9866,DS-89dc6e26-7219-
412d-a7fa-e33dbbb14cfe,DISK]]
```


#### Manually Fail a Block

We manually remove a block from one DataNode. For example, we manually fail
block 0 (named ```/test_code_oecobj_0``` in HDFS). **On the DataNode that
stores block 0**, we first enter the folder that physically stores the block
in HDFS:

```
cd ${HADOOP_HOME}/dfs/data/current/BP-*/current/finalized/subdir0/subdir0/
```

Only one block ```blk_<blkid>``` is stored with it's metadata file
```blk_<blkid>.meta```. We move the block to OpenEC project directory to
manually remove the block.

```
mv blk_<blkid> ~/openec
```

After the operation, block 0 is considered lost. HDFS will automatically
detect and report the lost block shortly after the block manual removal. We
can verify with ```hdfs fsck``` again.

```
hdfs fsck / -list-corruptfileblocks
```


#### Repair a Failed Block

After the failure is detected, **on the same DataNode**, we repair the lost
block 0 (named ```/test_code_0```, **without "\_oecobj\_"**) with the OpenEC
Client's read request, and store it as ```test_code_0```.

```
cd ~/openec
./OECClient read /test_code_0 test_code_0
```

We can verify that the repaired block is the same as the manually failed block
stored in ```~/openec/```.

```
cd ~/openec
cmp test_code_0 blk_<blkid>
```



### Run Full-node Recovery

Below is an example of full-node recovery for LESS with (14, 10),
sub-packetization = 4.  We write two stripes in OpenEC and manually remove the
two blocks stored in one DataNode.  We then trigger full-node recovery in
OpenEC to repair the blocks.

#### Write Blocks

On one HDFS DataNode:

- Use OpenEC Client to issue two (online) file writes. Assume the filename is
  ```input_640``` and the file size is 640 MiB in *k=10* HDFS blocks. The file
  is encoded into *n=14* HDFS blocks and randomly distributed to *n* storage
  nodes.

```
cd ~/openec
./OECClient write input_640 /S1_test_code LESS_14_10_4 online 640
./OECClient write input_640 /S2_test_code LESS_14_10_4 online 640
```

Similar to single-block repair, we can check the distribution of blocks for
the two stripes with:

```
hdfs fsck / -files -blocks -locations
```


#### Manually Fail all Blocks in One DataNode

Suppose we manually remove all blocks from DataNode 0:

```
rm -rf {}/dfs/data/current/BP*/current/finalized/*/*/blk_*
```

HDFS will automatically detect and report the lost blocks shortly after the
removal. We can verify with ```hdfs fsck``` again.

```
hdfs fsck / -list-corruptfileblocks
```


#### Repair a Failed Block

After the failures are detected, **on the same DataNode**, we use OpenEC
Client to trigger full-node recovery

```
cd ~/openec
./OECClient startRepair
```

We can verify in OpenEC Controller's log ```openec/coor_output``` when the full-node recovery is
completed.

```
cat openec/coor_output

// Sample log messages:
Coordinator::repair for <StripeName> finishes
Full-node recovery time: xxx seconds
```

### Misc: Run ECDAG Test

We provide ECDAG test cases in OpenEC.

For example, we test the encoding and decoding of ECDAG for LESS with (14,
10), sub-packetization = 4, and failed block 0:

```
./CodeTest LESS 14 10 4 1048576 0

// Sample outputs:
Repair degree (number of accessed nodes): 13
Repair bandwidth: total packets read: 19 / 40, average per node: 1.461538 (min: 1, max: 4), normalized repair bandwidth (w.t. RS): 0.475000
Repair access: total non-contiguous accesses: 13, average per node: 1.000000 (min: 1, max: 1)
```