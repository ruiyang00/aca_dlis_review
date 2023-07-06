# 1. Introduction

This repository contains the source code of our space ACA on data nodes, space ACA on internal nodes and time ACAs.

# 2. Contents

- Space ACA on data nodes
  - [MCK implementation](https://github.com/ruiyang00/aca_dlis_review/blob/master/attack/attack.h): This file contains main logic of the attack (MCK implementation)
  - Source code
    - [White-box](https://github.com/ruiyang00/aca_dlis_review/blob/master/src/benchmark/space_aca_data_node_whitebox.cpp)
    - [Gray-box](https://github.com/ruiyang00/aca_dlis_review/blob/master/src/benchmark/space_aca_data_node_graybox.cpp)
      - python [script](https://github.com/ruiyang00/aca_dlis_review/blob/master/scripts/populate_graybox_dataset.py) to generate dataset to 
 constrcut a "substitude" tree (described in paper).
  - Scripts to regenerate the result in Figure 2
    - [White-box](https://github.com/ruiyang00/aca_dlis_review/blob/master/scripts/run_space_aca_data_node_whitebox.sh)
    - [Gray-box](https://github.com/ruiyang00/aca_dlis_review/blob/master/scripts/run_space_aca_data_node_graybox.sh)
- Space ACA on internal nodes
  - Source code
    - [Black-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/src/benchmark/space_aca_internal_node_blackbox.cpp)
  - Script to regenerate the result in Table 1
    - [Black-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/scripts/run_space_aca_internal_node_blackbox.sh) 
- Time ACA on ALEX
  - Source code
    - [White-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/src/benchmark/time_aca_whitebox.cpp)
    - [Gray-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/src/benchmark/time_aca_graybox.cpp)
      - python [script](https://github.com/ruiyang00/aca_dlis_review/blob/master/scripts/populate_graybox_dataset.py) to generate dataset to constrcut a "substitude" tree. 
    - [Black-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/src/benchmark/time_aca_blackbox.cpp)
  - Scripts to regenerate the result in Figure 6 ( Note: Before mounting any time ACA attacks, we need to uncomment line 1274 to 1280 in [alex.h](https://github.com/ruiyang00/aca_dlis_review/blob/master/src/core/alex.h), as described in paper, to force ALEX to perform "catastrohpic expansion" instead of "catastrohpic split")
    - [White-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/scripts/run_time_aca_whitebox.sh)
    - [Gray-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/scripts/run_time_aca_graybox.sh)
    - [Black-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/scripts/run_time_aca_blackbox.sh)
    - [Legitmate workload](https://github.com/ruiyang00/aca_dlis_review/tree/master/scripts/run_time_aca_legit.sh)
- [Time ACA on APEX](https://github.com/ruiyang00/aca_dlis_review/tree/master/apex): This directory contains APEX source code, attack logic, and [script](https://github.com/ruiyang00/aca_dlis_review/blob/master/apex/run_time_aca.sh) to reproduce Figure 8 results.
- [ALEX](https://github.com/ruiyang00/aca_dlis_review/tree/master/src/core): ALEX's source code.

# 3. Dependencies for Attacks
- Space ACA on data nodes: [OR-Tools](https://developers.google.com/optimization/install) and [KDE](https://scikit-learn.org/stable/install.html)
- Space ACA on internal nodes: No dependencies
- Time ACA on ALEX: [KDE](https://scikit-learn.org/stable/install.html)
- Time ACA on APEX: This attack requires using DRAM to emulate the persistent memory (PM) through modifying the kernel configuration file. Please refer this [article](https://pmem.io/blog/2016/02/how-to-emulate-persistent-memory/) before mounting this attack.

# 4. Datasets
Datasets (Longitudes, Longlat, Lognormal, YCSB) used in this paper could be found in [ALEX](https://github.com/microsoft/ALEX) repo.


