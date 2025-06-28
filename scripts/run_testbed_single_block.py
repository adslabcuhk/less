import os
import time
import sys
import json
import argparse
import sys
import subprocess
from pathlib import Path
import numpy as np
import math
import configparser
import re

# import common configs
import common


def parseArgs(cmd_args):
    argParser = argparse.ArgumentParser(description="run testbed experiment for single-block repair") 

    argParser.add_argument("-f", type=str, help="Evaluation config file (.ini)", default=common.EXP_SCRIPT_DIR + "/settings.ini")
    
    args = argParser.parse_args(cmd_args)
    return args

def execCmd(cmd, exec=True, timeout=None, verbose=True):
    if verbose:
        print("Execute Command: {}".format(cmd))
    msg = ""
    success=False
    if exec == True:
        with subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE) as p:
            try:
                retStr, stderr = p.communicate(timeout=timeout)
                msg = retStr.decode().strip()
                success=True
            except Exception as e:
                print(e)
                p.terminate()
        print(msg)
    return msg, success

class MyConfigParser(configparser.ConfigParser):
    def __init__(self, defaults=None):
        configparser.ConfigParser.__init__(self, defaults=None)
    def optionxform(self, optionStr):
        return optionStr

class DictToObject:
    def __init__(self, dictionary):
        for key, value in dictionary.items():
            setattr(self, key, value)

def findBlockAndAddr(stripeName, blockId):
    
    nodeIp = "undefined_ip"
    blockName = "undefined_blk"
    oecBlockName = "/{}_oecobj_{}".format(stripeName, blockId)
    
    # call hdfs fsck to check the availability of the block
    cmd = "hdfs fsck {} -files -blocks -locations | grep Datanode".format(oecBlockName)
    findBlkResult, success = execCmd(cmd, exec=True)

    if not success:
        print("Error finding block for stripeName: {}".format(oecBlockName))
        return nodeIp, blockName

    try:
        findBlkResult.index("blk_")
    except ValueError:
        print("oec_object {} not found in HDFS!".format(oecBlockName))
        return nodeIp, blockName

    # find block name
    blockBegin = findBlkResult.index("blk_") + len("blk_")
    blockEnd = findBlkResult.index("len=")
    blockMeta = findBlkResult[blockBegin:blockEnd]
    blockMetaSplit = blockMeta.split("_")
    blockName = blockMetaSplit[0]

    # find node ip
    ipBegin = findBlkResult.index("WithStorage[") + len("WithStorage[")
    ipEnd = findBlkResult.index(",DS")
    ipOrigin = findBlkResult[ipBegin:ipEnd]
    ipOriginSplit = ipOrigin.split(":")
    nodeIp = ipOriginSplit[0]

    print("found block for stripeName: {}: {} {}".format(oecBlockName, nodeIp, blockName))

    return nodeIp, blockName

def deleteBlockFile(hadoopDir, nodeIp, blockName = "*"):
    print("Start to delete block: " + nodeIp + ", block: " + blockName)
    
    locate_cmd = "ssh " + nodeIp + " \"find {}/dfs/data/ -name blk_{}\"".format(hadoopDir, blockName)
    retVal, success = execCmd(locate_cmd, exec=True)

    delete_cmd = "ssh " + nodeIp + " \"rm {}\"".format(retVal)
    execCmd(delete_cmd, exec=True)

    print("Delete block finished: " + nodeIp + ", block: " + blockName)

def checkHDFSBlocks():
    cmd = "hdfs fsck -list-corruptfileblocks"
    retVal, success = execCmd(cmd, exec=True)
    # print(retVal)


def readFileBlock(userName, agentIp, projDir, readFileName, numRuns):
    print("Start to read file {} for {} runs".format(readFileName, numRuns))

    numSuccessReads = 0

    readTimeList = []
    for i in range(numRuns):
        read_cmd = "ssh {}@{} \"cd {} && ./OECClient read {} {}\"".format(userName, agentIp, projDir, "/" + readFileName, readFileName)
        retVal, success = execCmd(read_cmd, exec=True, timeout=300)

        if not success:
            print("Error: timeout reading object {} for the {}-th run".format(readFileName, i))
            break

        readTime = -1

        try:
            retVal.index("duration:")
        except ValueError:
            print("Error reading object {}".format(readFileName))
            continue

        match = re.search(r".*read.overall.duration: (\d+\.\d+|\d+)", retVal)
        if not match or not match.groups():
            print("Error matching the results {}".format(readFileName))
            continue
        # print(line)
        readTime = float(match.group(1))

        numSuccessReads += 1
        readTimeList.append(readTime)

        time.sleep(2)

    if numSuccessReads == numRuns:
        return readTimeList, True
    else:
        return readTimeList, False


def main():
    args = parseArgs(sys.argv[1:])
    if not args:
        exit()

    startExpTime = time.time()

    # Input parameters: exp_settings_file
    evalSettingsFile = args.f

    # Load configurations
    configsRaw = MyConfigParser()
    configsRaw.read(evalSettingsFile)

    # parse configurations
    configs = DictToObject({section: dict(configsRaw[section]) for section in configsRaw.sections()})

    # experiments
    configs.Experiment = DictToObject(configs.Experiment)
    exp = configs.Experiment
    exp.code_test_list_file = str(exp.code_test_list_file)

    # extract codes
    exp.codes = []
    with open(exp.code_test_list_file, 'r') as f:
        for line in f.readlines():
            if len(line.strip()) == 0:
                continue
            # format: code_name ecn eck ecw
            code_params = line.strip().split(' ')
            exp.codes.append(code_params)

    exp.num_runs = int(exp.num_runs)

    # cluster settings
    configs.Cluster = DictToObject(configs.Cluster)
    cluster = configs.Cluster
    cluster.bandwidth_kbps = int(cluster.bandwidth_kbps)
    cluster.controller_id = int(cluster.controller_id)
    cluster.agent_ids = list(map(int, cluster.agent_ids.split(',')))
    # load nodes
    cluster.nodeIps=[]
    # load node IP
    with open(cluster.node_list_file, 'r') as f:
        for line in f.readlines():
            if len(line.strip()) == 0:
                continue
            # format: nodeIp
            nodeIp = line.strip()
            cluster.nodeIps.append(nodeIp)
    cluster.num_nodes=len(cluster.nodeIps)

    # openec configs
    configs.OpenEC = DictToObject(configs.OpenEC)
    openec = configs.OpenEC
    openec.block_size_byte = int(openec.block_size_byte)
    openec.packet_size_byte = int(openec.packet_size_byte)
    blockSizeMiB = int(openec.block_size_byte / 1024 / 1024)
    packetSizeKiB = int(openec.packet_size_byte / 1024)

    print("Configurations:")
    print(vars(exp))
    print(vars(cluster))
    print(vars(openec))

    # clear network bandwidth
    cmd = "cd {} && bash clear_bw.sh".format(common.EXP_SCRIPT_DIR)
    execCmd(cmd, exec=True)
    cmd = "cd {} && bash run_script_dist.sh clear_bw.sh".format(common.EXP_SCRIPT_DIR)
    execCmd(cmd, exec=True)

    # update configurations
    cmd = "cd {} && bash -i update_config_dist.sh {} {} {}".format(common.EXP_SCRIPT_DIR, openec.block_size_byte, openec.packet_size_byte, cluster.bandwidth_kbps)
    execCmd(cmd, exec=True)

    # restart hdfs and openec
    cmd = "cd {} && bash restart_oec.sh".format(common.EXP_SCRIPT_DIR)
    execCmd(cmd, exec=True)

    time.sleep(1)
    
    for code_params in exp.codes:
        codeName = code_params[0]
        ecn = int(code_params[1])
        eck = int(code_params[2])
        ecw = int(code_params[3])
        codeId = "{}_{}_{}_{}".format(codeName, ecn, eck, ecw)

        print("Start evaluating code {}".format(codeId))

        # evaluation workflow: (1) write n stripes; (2) delete the i-th block
        # from the i-th stripe; (3) repair the i-th block from the i-th stripe
        # at the corresponding node; (4) repeat for all n stripes

        print("Write {} stripes".format(ecn))
        # generate input file on the first data node
        inputFileSizeMiB = blockSizeMiB * eck
        inputFileName = "{}/{}MiB".format(common.PROJ_DIR, inputFileSizeMiB)
        cmd = "ssh {}@{} \"test -f {} && echo yes || echo no\"".format(cluster.user_name, cluster.nodeIps[cluster.agent_ids[0]], inputFileName)
        retVal, success = execCmd(cmd, exec=True)
        if "yes" in retVal:
            print("Input file exists: {}; skip generating".format(inputFileName))
        else:
            print("Input file not exists: {}; generate".format(inputFileName))
            cmd = "ssh {}@{} \"dd if=/dev/urandom of={} bs={}MiB count={} iflag=fullblock\"".format(cluster.user_name, cluster.nodeIps[cluster.agent_ids[0]], inputFileName, blockSizeMiB, eck)
            execCmd(cmd, exec=True)

        # write n stripes on the first data node
        for blockId in range(ecn):
            print("Write {}-th stripe for code {}".format(blockId, codeId))
            stripeName = "_".join(["Stripe", str(blockId), codeId])
            cmd = "ssh {}@{} \"cd {} && ./OECClient write {} {} {} online {}\"".format(cluster.user_name, cluster.nodeIps[cluster.agent_ids[0]], common.PROJ_DIR, inputFileName, "/" + stripeName, codeId, inputFileSizeMiB)
            execCmd(cmd, exec=True)
            time.sleep(10)

        # reset network bandwidth
        cmd = "cd {} && bash clear_bw.sh".format(common.EXP_SCRIPT_DIR)
        execCmd(cmd, exec=True)
        cmd = "cd {} && bash run_script_dist.sh clear_bw.sh".format(common.EXP_SCRIPT_DIR)
        execCmd(cmd, exec=True)

        cmd = "cd {} && bash set_bw.sh {}".format(common.EXP_SCRIPT_DIR, cluster.bandwidth_kbps)
        execCmd(cmd, exec=True)
        cmd = "cd {} && bash run_script_dist.sh set_bw.sh {}".format(common.EXP_SCRIPT_DIR, cluster.bandwidth_kbps)
        execCmd(cmd, exec=True)

        # repair the i-th block in the i-th stripe
        blockId = 0
        while blockId < ecn:
            print("Start evaluating code {}, repair block {}".format(codeId, blockId))

            stripeName = "_".join(["Stripe", str(blockId), codeId])

            # find block and node ip
            nodeIp, blockFileName = findBlockAndAddr(stripeName, blockId)

            # delete node block
            deleteBlockFile(common.HADOOP_DIR, nodeIp, blockFileName)

            time.sleep(10)
            # check if the block is corrupted
            checkHDFSBlocks()

            # degraded read file
            readFileName = "{}_{}".format(stripeName, blockId)
            readTimeList, success = readFileBlock(cluster.user_name, nodeIp, common.PROJ_DIR, readFileName, exp.num_runs)

            # # if not success, retry experiment for this block
            # if not success:
            #     print("Warning: read file block failed, retry experiment for block {}\n\n".format(blockId))
            #     continue

            # save results
            resultSaveFolder = "{}/eval_results/single/{}/bw{}Gbps_blk{}MiB_pkt{}KiB".format(common.PROJ_DIR, codeId, int(cluster.bandwidth_kbps / 1024 / 1024), blockSizeMiB, packetSizeKiB)
            Path(resultSaveFolder).mkdir(parents=True, exist_ok=True)
            resultSavePath = resultSaveFolder + "/block_{}.txt".format(blockId)
            with open(resultSavePath, 'w',encoding='utf8') as f:
                f.write(" ".join(str(item) for item in readTimeList) + "\n")
            print("results for code {} block {}: {}".format(codeId, blockId, " ".join(str(item) for item in readTimeList)))    
            print("save {} result in {}".format(codeId, resultSavePath))

            # increment blockId
            blockId += 1

            print("Finished evaluation for code {}, block {}\n".format(codeId, blockId))

        print("Finished evaluating code {}\n".format(codeId))

        # clear network bandwidth
        cmd = "cd {} && bash clear_bw.sh".format(common.EXP_SCRIPT_DIR)
        execCmd(cmd, exec=True)
        cmd = "cd {} && bash run_script_dist.sh clear_bw.sh".format(common.EXP_SCRIPT_DIR)
        execCmd(cmd, exec=True)

    end_exp_time = time.time()
    print("Evaluation finished, used time: {} (s)".format(str(end_exp_time - startExpTime)))

if __name__ == '__main__':
    main()


