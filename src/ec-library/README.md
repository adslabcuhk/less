# Erasure Coding Library


## Overview

This repository contains a standalone erasure coding library written in C++ to
test encoding and decoding performance of different erasure codes.  The coding
operations supports both [Jerasure](https://github.com/ceph/jerasure) (by
default) and [ISA-L](https://github.com/intel/isa-l) (for GF(2^8) only).

The library includes the following erasure codes:

* **Reed-Solomon (RS) codes**: Vandermonde-based RS codes based on the
  Vandermonde parity check matrix.
* **LESS**: We implement LESS as described in the paper.


## Installation

The library can be installed in Ubuntu 22.04.


* Install dependencies

```
apt-get install libtool autoconf
```

* Compile the library

```
mkdir build && cd build
cmake .. && make
```

## Run

Run CodeTest to test the encoding and decoding of an (n,k) erasure code. We
encode a single stripe in memory, where the data blocks are filled with random bytes.
We specify the failed blocks, then the library will repair the block and verify
the correctness of decoded data.

Example 1: encode RS(n=14,k=10) with 1 MiB block size, and repair block 0:

```
./CodeTest RSCode 14 10 1 1048576 0
```


Example 2: encode LESS(n=14,k=10,alpha=2) with 256 KiB block size, and repair
block 3:

```
./CodeTest LESS 14 10 2 262144 3
```
