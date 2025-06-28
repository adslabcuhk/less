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
    argParser = argparse.ArgumentParser(description="run testbed experiment for full-node recovery") 

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

def checkHDFSBlocks():
    cmd = "hdfs fsck -list-corruptfileblocks"
    retVal, success = execCmd(cmd, exec=True)
    # print(retVal)

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

    exp.full_node_num_stripes = int(exp.full_node_num_stripes)
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

    cmd = "pkill -9 -f \"get_full_node_recovery_time.sh\""
    execCmd(cmd, exec=True)

    # clear network bandwidth
    cmd = "cd {} && bash clear_bw.sh".format(common.EXP_SCRIPT_DIR)
    execCmd(cmd, exec=True)
    cmd = "cd {} && bash run_script_dist.sh clear_bw.sh".format(common.EXP_SCRIPT_DIR)
    execCmd(cmd, exec=True)

    # update configurations
    cmd = "cd {} && bash -i update_config_dist.sh {} {} {}".format(common.EXP_SCRIPT_DIR, openec.block_size_byte, openec.packet_size_byte, cluster.bandwidth_kbps)
    execCmd(cmd, exec=True)

    time.sleep(1)
 
    for runId in range(exp.num_runs):
        print("Start run {}".format(runId))
   
        for code_params in exp.codes:
            codeName = code_params[0]
            ecn = int(code_params[1])
            eck = int(code_params[2])
            ecw = int(code_params[3])
            codeId = "{}_{}_{}_{}".format(codeName, ecn, eck, ecw)

            print("Start evaluating code {}".format(codeId))
            # evaluation workflow: (1) write <exp.full_node_num_stripes>
            # stripes; (2) delete all blocks on the 1st node (all blocks are
            # randomly distributed); (3) start full node repair (4) obtain the
            # time

            # restart hdfs and openec
            cmd = "cd {} && bash restart_oec.sh".format(common.EXP_SCRIPT_DIR)
            execCmd(cmd, exec=True)

            print("Write {} stripes".format(exp.full_node_num_stripes))
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

            # write <exp.full_node_num_stripes> stripes on the first data node
            for stripeId in range(exp.full_node_num_stripes):
                print("Write {}-th stripe for code {}".format(stripeId, codeId))
                stripeName = "_".join(["Stripe", str(stripeId), codeId])
                cmd = "ssh {}@{} \"cd {} && ./OECClient write {} {} {} online {}\"".format(cluster.user_name, cluster.nodeIps[cluster.agent_ids[0]], common.PROJ_DIR, inputFileName, "/" + stripeName, codeId, inputFileSizeMiB)
                execCmd(cmd, exec=True)
                time.sleep(10)

            # remove all blocks on the 1st data node
            print("Remove all blocks on the 1st data node")
            nodeIp = cluster.nodeIps[cluster.agent_ids[0]]
            cmd = "ssh {}@{} \"rm -rf {}/dfs/data/current/BP*/current/finalized/*/*/blk_*\"".format(cluster.user_name, nodeIp, common.HADOOP_DIR)
            execCmd(cmd, exec=True)
            time.sleep(30)

            # check if the block is corrupted
            checkHDFSBlocks()

            # reset network bandwidth
            cmd = "cd {} && bash clear_bw.sh".format(common.EXP_SCRIPT_DIR)
            execCmd(cmd, exec=True)
            cmd = "cd {} && bash run_script_dist.sh clear_bw.sh".format(common.EXP_SCRIPT_DIR)
            execCmd(cmd, exec=True)

            cmd = "cd {} && bash set_bw.sh {}".format(common.EXP_SCRIPT_DIR, cluster.bandwidth_kbps)
            execCmd(cmd, exec=True)
            cmd = "cd {} && bash run_script_dist.sh set_bw.sh {}".format(common.EXP_SCRIPT_DIR, cluster.bandwidth_kbps)
            execCmd(cmd, exec=True)

            # monitor the coordinator output and fetch the recovery time
            monitorCmd = "bash get_full_node_recovery_time.sh"
            msg = ""
            timeout = 300
            try:
                proc = subprocess.Popen(f"{monitorCmd}", shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

                # full node repair
                print("Start full node repair")
                cmd = "ssh {}@{} \"cd {} && ./OECClient startRepair\"".format(cluster.user_name, nodeIp, common.PROJ_DIR)
                execCmd(cmd, exec=True)

                # Check periodically if the background process is still
                # running
                pid = proc.pid
                print("waiting for the results, pid: {}".format(pid))
                
                # Once the process has finished, read the output
                retStr, stderr = proc.communicate()
                msg = retStr.decode().strip()
                success = True
            except Exception as e:
                print(f"Error executing command: {e}")
                msg = str(e)
                success = False
            
            if success:
                # save results
                resultSaveFolder = "{}/eval_results/full-node/{}/bw{}Gbps_blk{}MiB_pkt{}KiB".format(common.PROJ_DIR, codeId, int(cluster.bandwidth_kbps / 1024 / 1024), blockSizeMiB, packetSizeKiB)
                Path(resultSaveFolder).mkdir(parents=True, exist_ok=True)
                resultSavePath = resultSaveFolder + "/recovery_run_{}.txt".format(runId)
                with open(resultSavePath, 'w',encoding='utf8') as f:
                    f.write(msg + "\n")   
                print("save {} result in {}".format(codeId, resultSavePath))
            else:
                print("Error waiting for the full-node recovery result: {}".format(msg))

            # clear network bandwidth
            cmd = "cd {} && bash clear_bw.sh".format(common.EXP_SCRIPT_DIR)
            execCmd(cmd, exec=True)
            cmd = "cd {} && bash run_script_dist.sh clear_bw.sh".format(common.EXP_SCRIPT_DIR)
            execCmd(cmd, exec=True)

            print("finished run {}".format(runId))
        
        print("Finished evaluating code {}\n".format(codeId))

    end_exp_time = time.time()
    print("Evaluation finished, used time: {} (s)".format(str(end_exp_time - startExpTime)))

if __name__ == '__main__':
    main()



