import os
import time
import sys
import argparse
import sys
import subprocess
import re
import configparser

# import common configs
import common

DEFAULT_BLOCK_SIZE = 1024

# Evaluation metric: "arc" (average repair cost) or "adrc" (average degraded read cost)
metric = "arc"

codeList = [
("RSCONV",14,10,1),
("Clay",14,10,256),
("HHXORPlus",14,10,2),
("HTEC",14,10,2),
("HTEC",14,10,3),
("HTEC",14,10,4),
("ETRSConv",14,10,2),
("ETRSConv",14,10,3),
("ETRSConv",14,10,4),
("LESS",14,10,2),
("LESS",14,10,3),
("LESS",14,10,4),
("RSCONV",80,76,1),
("RSCONV",100,96,1),
("RSCONV",124,120,1),
("LESS",80,76,4),
("LESS",100,96,4),
("LESS",124,120,4),
]

def parseArgs(cmdArgs):
    argParser = argparse.ArgumentParser(description="Exp#A1: Single-block repair")

    argParser.add_argument("-f", type=str, help="Evaluation config file (.ini)", default=common.EXP_SCRIPT_DIR + "/settings.ini")
    
    args = argParser.parse_args(cmdArgs)
    return args

def execCmd(cmd, exec=True, timeout=None, printCmd=True, printOutputs=True):
    if printCmd == True:
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
        if printOutputs == True:
            print(msg)
    return msg, success

class DictToObject:
    def __init__(self, dictionary):
        for key, value in dictionary.items():
            setattr(self, key, value)

def main():
    args = parseArgs(sys.argv[1:])
    if not args:
        exit()

    # Input parameters: configFile
    configFile = args.f

    print("Exp#A1: Single-block repair\n")
    print("Code List:", codeList)

    # parse configs
    configsRaw = configparser.ConfigParser()
    configsRaw.read(configFile)
    configs = DictToObject({section: dict(configsRaw[section]) for section in configsRaw.sections()})

    experiment = DictToObject(configs.Experiment)
    codeListFile = experiment.code_test_list_file
    
    # dump codeList to codeListFile
    with open(codeListFile, 'w') as f:
        for code in codeList:
            codeName = code[0]
            codeN = code[1]
            codeK = code[2]
            codeW = code[3]
            f.write("{} {} {} {}\n".format(codeName, codeN, codeK, codeW))

    startExpTime = time.time()

    # generate OpenEC configs with codeList
    print("Generate OpenEC configs")
    genOECConfigScript = os.path.join(common.EXP_SCRIPT_DIR, "gen_oec_config.py")
    cmd = "python3 {} -f {}".format(genOECConfigScript, common.EVAL_SETTING_FILE)
    execCmd(cmd)

    print("Run OpenEC CodeTest for each code")
    for code in codeList:
        codeName = code[0]
        codeN = code[1]
        codeK = code[2]
        codeW = code[3]
        codeId = "_".join([codeName, str(codeN), str(codeK), str(codeW)])

        # Repair degree stats
        repairDegStats = []

        # Repair bandwidth stats
        repairBWStats = []
        repairBWTotalStats = []
        repairTransferNodeStats = []
        AllMinRetSubPktNode = codeW
        AllMaxRetSubPktNode = 0

        # Repair access
        nonContIOStats = []
        nonContIONodeStats = []
        AllMinNonContAccessNode = codeW
        AllMaxNonContAccessNode = 0

        maxBlockId = codeN
        # degraded read cost
        if metric == "adrc":
            maxBlockId = codeK
        # 
        elif metric == "arc":
            maxBlockId = codeN
        for failedNodeId in range(maxBlockId):
            cmd = "cd {} && ./{} {} {} {} {} {} {}".format(common.PROJ_DIR, "CodeTest", codeName, codeN, codeK, codeW, DEFAULT_BLOCK_SIZE, failedNodeId)
            msg, success = execCmd(cmd, printCmd=False, printOutputs=False)

            if "error" in msg:
                print("Error: invalid outputs: {}".format((msg)))
                exit(-1)
            
            # extract the repair degree
            resultToken = "Repair degree"
            for line in msg.split('\n'):
                line = line.strip()
                if len(line) == 0:
                    continue
                if resultToken not in line:
                    continue
                match = re.search(r"{} .*: (\d+)".format(resultToken), line)
                if not match:
                    print("Error: cannot find result from line: {}".format(line))
                    # print(msg)
                    exit()
                repairDegree = match.group(1)

                repairDegStats.append(int(repairDegree))
            
            # extract the repair bandwidth
            resultToken = "Repair bandwidth"
            for line in msg.split('\n'):
                line = line.strip()
                if len(line) == 0:
                    continue
                if resultToken not in line:
                    continue
                match = re.search(r"{}: total packets read: (\d+) / (\d+), average per node: (\d*\.\d+) \(min: (\d+), max: (\d+)\).*".format(resultToken), line)
                if not match:
                    print("Error: cannot find result from line: {}".format(line))
                    # print(msg)
                    exit()
                numRetSubPkts = match.group(1)
                numAllSubPkts = match.group(2)
                avgRetSubPktPerNode = match.group(3)
                minRetSubPktNode = match.group(4)
                maxRetSubPktNode = match.group(5)

                repairBWStats.append(int(numRetSubPkts))
                repairBWTotalStats.append(int(numAllSubPkts))
                repairTransferNodeStats.append(float(avgRetSubPktPerNode))
                AllMinRetSubPktNode = min(AllMinRetSubPktNode, int(minRetSubPktNode))
                AllMaxRetSubPktNode = max(AllMaxRetSubPktNode, int(maxRetSubPktNode))

            # extract the repair access
            resultToken = "Repair access"
            for line in msg.split('\n'):
                line = line.strip()
                if len(line) == 0:
                    continue
                if resultToken not in line:
                    continue
                match = re.search(r"{}: total non-contiguous accesses: (\d+), average per node: (\d*\.\d+) \(min: (\d+), max: (\d+)\)".format(resultToken), line)
                if not match:
                    print("Error: cannot find result from line: {}".format(line))
                    # print(msg)
                    exit()
                numNonContAccess = match.group(1)
                avgNonContAccessPerNode = match.group(2)
                minNonContAccessNode = match.group(3)
                maxNonContAccessNode = match.group(4)

                nonContIOStats.append(int(numNonContAccess))
                nonContIONodeStats.append(float(avgNonContAccessPerNode))
                AllMinNonContAccessNode = min(AllMinNonContAccessNode, int(minNonContAccessNode))
                AllMaxNonContAccessNode = max(AllMaxNonContAccessNode, int(maxNonContAccessNode))

        # Repair access

        # validate if results are correct
        if sum(repairBWTotalStats) != maxBlockId * codeW * codeK:
            print("Error: invalid number of total subpackets: {} != {}".format(sum(repairBWTotalStats), int(maxBlockId * codeW * codeK)))
            exit(-1)

        # print stats
        print("")
        print("Code: {} ({}, {}, {})".format(codeName, codeN, codeK, codeW))
        print("Repair I/O (bandwidth): avg: {}, min: {}, max: {}".format(sum(repairBWStats) / codeW / maxBlockId, min(repairBWStats) / codeW, max(repairBWStats) / codeW))
        print("I/O seeks: avg: {}, min: {}, max: {}".format(sum(nonContIOStats) / maxBlockId, min(nonContIOStats), max(nonContIOStats)))
        print("")

    endExpTime = time.time()
    print("Test finished, used time: {} (s)".format(str(endExpTime - startExpTime)))

if __name__ == '__main__':
    main()


