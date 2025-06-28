import os
import sys
import subprocess
import argparse
import time
import random
import numpy as np
from pathlib import Path
import numpy as np
from scipy.stats import t
import re

def parseArgs(cmd_args):
    arg_parser = argparse.ArgumentParser(description="extract logs from testbed experiments for single-block repair")
    arg_parser.add_argument("-ecn", type=int, required=True, help="ecn")
    arg_parser.add_argument("-r", type=int, required=True, help="number of runs")
    arg_parser.add_argument("-d", type=str, required=True, help="log directory. The directory contains <ecn> .txt files for the testbed experiments")
    
    args = arg_parser.parse_args(cmd_args)
    return args

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

    ecn = args.ecn
    numRuns = args.r
    logDir = args.d 
    logDir = Path(logDir).resolve()

    # print("log directory: {}".format(logDir))

    results = [[] for i in range(numRuns)]

    for i in range(ecn):
        resultFileName = "{}/block_{}.txt".format(logDir, i)
        with open(resultFileName, "r") as f:
            resultRawStr = ""
            for line in f.readlines():
                resultRawStr += line
            # split by spaces
            blkResults = [float(item) for item in resultRawStr.strip().split(" ")]
            if len(blkResults) != numRuns:
                print("error: insufficient number of runs for {}".format(resultFileName))
            for j in range(numRuns):
                results[j].append(blkResults[j])

    results = [sum(item) / len(item) / 1000 for item in results]

    results_std_t = student_t_dist(results)
    resultsAvg = results_std_t[0]
    resultsLower = results_std_t[0] - results_std_t[1]
    resultsUpper = results_std_t[0] + results_std_t[1]

    print("Single-block repair time (sec): avg: {}, lower: {}, upper: {}".format(resultsAvg, resultsLower, resultsUpper))
    # print("Raw: {}".format(results))

if __name__ == '__main__':
    main()
