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
from scipy.stats import t

# import common configs
import common

numRuns = 10
codeParams=[("RSCode", 9, 6, 1),("RSCode", 14, 10, 1),("RSCode", 16, 12, 1),("LESS", 9, 6, 2),("LESS", 9, 6, 3),("LESS", 14, 10, 2),("LESS", 14, 10, 3),("LESS", 14, 10, 4),("LESS", 16, 12, 2),("LESS", 16, 12, 3),("LESS", 16, 12, 4),("LESS", 124, 120, 4)]
evalBlockSizeBytes=[131072, 262144, 524288, 1048576]

RAW_LIB_DIR=common.PROJ_DIR + "/src/ec-library"
RAW_LIB_BUILD_DIR=RAW_LIB_DIR + "/build"


def parseArgs(cmdArgs):
    argParser = argparse.ArgumentParser(description="Exp#A1: Single-block repair")

    argParser.add_argument("-f", type=str, help="Evaluation config file (.ini)", default=common.EXP_SCRIPT_DIR + "/settings.ini")
    
    args = argParser.parse_args(cmdArgs)
    return args

class DictToObject:
    def __init__(self, dictionary):
        for key, value in dictionary.items():
            setattr(self, key, value)

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
        # print(msg)
    return msg, success

def student_t_dist(samples_arr, ci=0.95):
    samples = np.array(samples_arr)
    # sample mean
    mean = samples.mean()
    # sample standard deviation
    stdev = samples.std()
    # degree of freedom = # of samples - 1
    dof = len(samples) - 1
    # t value
    t_crit = np.abs(t.ppf((1-ci) / 2, dof))
    # print('t = {:.3f} when percentage = {:.3f} and degree of freedom = {:d}'.format(t_crit, (1-ci)*100/2, dof))
    # interval
    t_diff = stdev * t_crit / np.sqrt(len(samples))

    return mean, t_diff


def main():
    args = parseArgs(sys.argv[1:])
    if not args:
        exit()

    # Input parameters: configFile
    configFile = args.f

    print("Exp#B3: Encoding throughput")

    # parse configs
    configsRaw = configparser.ConfigParser()
    configsRaw.read(configFile)
    configs = DictToObject({section: dict(configsRaw[section]) for section in configsRaw.sections()})

    experiment = DictToObject(configs.Experiment)
    numRuns = int(experiment.num_runs)
    
    startExpTime = time.time()

    time.sleep(1)
    
    for codeParam in codeParams:
        for blockSizeBytes in evalBlockSizeBytes:
            codeName = codeParam[0]
            codeN = codeParam[1]
            codeK = codeParam[2]
            codeAlpha = codeParam[3]

            encodingThroughputList = []
            encodingTimeList = []

            decodingThroughputList = []
            decodingTimeList = []

            for i in range(numRuns):
                runEncThptList = []
                runEncTimeList = []
                runDecThptList = []
                runDecTimeList = []
                # for blockId in range(int(codeN)):
                for blockId in range(1):
                    cmd = "cd {} && ./CodeTest {} {} {} {} {} {}".format(RAW_LIB_BUILD_DIR, codeName, codeN, codeK, codeAlpha, blockSizeBytes, blockId)
                    retVal, success = execCmd(cmd, exec=True, verbose=False)
                    # extract Encoding throughput from retVal
                    # format: Encoding throughput: 1464.843750 MiB/s, time:
                    # 0.000004
                    resultToken = "Encoding throughput:"
                    for line in retVal.split("\n"):
                        line = line.strip()
                        if line.startswith(resultToken):
                            match = re.search(r"{} (\d+\.\d+) MiB/s, time: (\d+\.\d+)".format(resultToken), line)
                            if not match:
                                print("Error: cannot find result from line: {}".format(line))
                                # print(msg)
                                exit()
                            encodingThroughput = match.group(1)
                            encodingTime = match.group(2)
                            runEncThptList.append(float(encodingThroughput))
                            runEncTimeList.append(float(encodingTime))
                            break

                    # extract Decoding throughput from retVal
                    # format: Decoding throughput: 1464.843750 MiB/s, time:
                    resultToken = "Decoding throughput:"
                    for line in retVal.split("\n"):
                        line = line.strip()
                        if line.startswith(resultToken):
                            match = re.search(r"{} (\d+\.\d+) MiB/s, time: (\d+\.\d+)".format(resultToken), line)
                            if not match:
                                print("Error: cannot find result from line: {}".format(line))
                                # print(msg)
                                exit()
                            decodingThroughput = match.group(1)
                            decodingTime = match.group(2)
                            runDecThptList.append(float(decodingThroughput))
                            runDecTimeList.append(float(decodingTime))
                            break
                
                # average over all blocks
                avgEncThpt = sum(runEncThptList) / len(runEncThptList)
                avgEncTime = sum(runEncTimeList) / len(runEncTimeList)
                encodingThroughputList.append(avgEncThpt)
                encodingTimeList.append(avgEncTime)

                # average over all blocks
                avgDecThpt = sum(runDecThptList) / len(runDecThptList)
                avgDecTime = sum(runDecTimeList) / len(runDecTimeList)
                decodingThroughputList.append(avgDecThpt)
                decodingTimeList.append(avgDecTime)
            
            # calculate average encoding throughput
            enc_results_std_t = student_t_dist(encodingThroughputList)
            encResultsAvg = enc_results_std_t[0]
            encResultsLower = enc_results_std_t[0] - enc_results_std_t[1]
            encResultsUpper = enc_results_std_t[0] + enc_results_std_t[1]

            print("{} ({}, {}, {}), packet size (bytes): {}, Encoding throughput (MiB/s): avg: {}, lower: {}, upper: {}".format(codeParam[0], codeParam[1], codeParam[2], codeParam[3], blockSizeBytes, encResultsAvg, encResultsLower, encResultsUpper))
            print("\n")
            # print("Raw results: {}".format(encodingThroughputList))

            # # calculate average decoding throughput
            # dec_results_std_t = student_t_dist(decodingThroughputList)
            # decResultsAvg = dec_results_std_t[0]
            # decResultsLower = dec_results_std_t[0] - dec_results_std_t[1]
            # decResultsUpper = dec_results_std_t[0] + dec_results_std_t[1]

            # print("Decoding results for {}: {} {} {}".format(codeParam, decResultsAvg, decResultsLower, decResultsUpper))
            # print("Raw results: {}".format(decodingThroughputList))
            # time.sleep(1)

    end_exp_time = time.time()
    print("Evaluation finished, used time: {} (s)".format(str(end_exp_time - startExpTime)))

if __name__ == '__main__':
    main()


