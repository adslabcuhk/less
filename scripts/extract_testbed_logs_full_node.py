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
    arg_parser = argparse.ArgumentParser(description="extract logs from testbed experiments for full-node recovery")
    arg_parser.add_argument("-r", type=int, required=True, help="number of runs")
    arg_parser.add_argument("-d", type=str, required=True, help="log directory. The directory contains #runs .txt files for the testbed experiments")
    
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

    numRuns = args.r
    logDir = args.d 
    logDir = Path(logDir).resolve()

    # print("log directory: {}".format(logDir))

    results = []

    resultToken = "Full-node recovery time:"

    for i in range(numRuns):
        resultFileName = "{}/recovery_run_{}.txt".format(logDir, i)
        with open(resultFileName, "r") as f:
            for line in f.readlines():
                if resultToken not in line:
                    continue
                # split by spaces
                line = line.strip()
                match = re.search(r"{}\s*([\d.]+)\s*seconds".format(resultToken), line)
                if match:
                    result = float(match.group(1))
                    results.append(result)

    results_std_t = student_t_dist(results)
    resultsAvg = results_std_t[0]
    resultsLower = results_std_t[0] - results_std_t[1]
    resultsUpper = results_std_t[0] + results_std_t[1]

    print("Full-node recovery time (sec): avg: {}, lower: {}, upper: {}".format(resultsAvg, resultsLower, resultsUpper))
    # print("Raw: {}".format(results)) 

if __name__ == '__main__':
    main()
