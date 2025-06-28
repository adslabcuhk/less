# LESS

## Overview

LESS is a family of erasure codes designed for I/O-efficient repairs by
reducing both repair I/O and I/O seeks and balancing the reductions across all
blocks.  This repository contains the implementation of LESS as described in
our USENIX FAST 2026 paper.

## Publications

Keyun Cheng*, Guodong Li*, Xiaolu Li, Sihuang Hu, and Patrick P. C. Lee. 
"LESS is More for I/O-Efficient Repairs in Erasure-Coded Storage."
USENIX FAST 2026. (*: Equal contribution)

## Overview

This repository contains the following components:

* **Erasure coding library** (```src/ec-library```): A standalone erasure coding
  library for testing encoding and decoding computation performance.
* **Finding primitive element** (```src/less-element-search```): The brute-force search implementation of finding the primitive element for LESS
coding coefficients.
* **OpenEC patch** (```src/openec-patch```): We implement LESS atop [HDFS 3.3.4](https://hadoop.apache.org/docs/r3.3.4/)
  with [OpenEC](https://www.usenix.org/conference/fast19/presentation/li)
  (USENIX FAST 2019). The directory contains the patch to OpenEC to support LESS.
* **Artifact evaluation instructions** (```AE_INSTRUCTION.md```): Instructions for evaluating the artifact.
* **Artifact evaluation scripts** (```scripts```): Scripts for reproducing
  results in our paper.

For details, please refer to the ```README.md``` in corresponding
subdirectories.

## Getting Started Instructions

To quickly verify the effectiveness of LESS, we suggest to check our
implementation of LESS in the [erasure coding library](src/ec-library), or
refer to the quick start experiments in [```AE_INSTRUCTION.md```](AE_INSTRUCTION.md).

## Detailed Instructions

To reproduce our results in the paper, please refer to the [```AE_INSTRUCTION.md```](AE_INSTRUCTION.md).

## Questions

Please contact Keyun Cheng (```kycheng@cse.cuhk.edu.hk```) for any questions.
