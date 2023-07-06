# 1. Introduction

This repository contains the source code of our space ACA on data nodes, space ACA on internal nodes and time ACAs.

# 2. Contents

- Space ACA on data nodes
  - [MCK implementation]((https://github.com/ruiyang00/aca_dlis_review/tree/master/attack)): This directry contains data node attack logic (MCK implementation)
  - Source code
    - [White-box](https://github.com/ruiyang00/aca_dlis_review/blob/master/src/benchmark/space_aca_data_node_whitebox.cpp)
    - [Gray-box](https://github.com/ruiyang00/aca_dlis_review/blob/master/src/benchmark/space_aca_data_node_graybox.cpp)
  - Scripts to regenerate the result in Figure 2
    - [White-box](https://github.com/ruiyang00/aca_dlis_review/blob/master/scripts/run_space_aca_data_node_whitebox.sh)
    - [Gray-box](https://github.com/ruiyang00/aca_dlis_review/blob/master/scripts/run_space_aca_data_node_graybox.sh)
- [Space ACA on internal nodes](https://github.com/ruiyang00/aca_dlis_review/tree/master/src/benchmark): The source code for space ACA on internal nodes.
  - Source code
    - [Black-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/src/benchmark/space_aca_internal_node_blackbox.cpp)
  - Script to regenerate the result in Table 1
    - [Black-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/scripts/run_space_aca_internal_node_blackbox.sh) 
- Time ACA on ALEX: The source code for time ACA on ALEX
  - Source code
    - [White-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/src/benchmark/time_aca_whitebox.cpp)
    - [Gray-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/src/benchmark/time_aca_graybox.cpp)
    - [Black-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/src/benchmark/time_aca_blackbox.cpp)
  - Scripts to regenerate the result in Figure 6 ( Note: Before mounting any time ACA attacks, we need to uncomment line 1274 to 1280 in [alex.h](https://github.com/ruiyang00/aca_dlis_review/blob/master/src/core/alex.h), as described in, to force ALEX to perform "catastrohpic expansion" instead of "catastrohpic split")
    - [White-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/scripts/run_time_aca_whitebox.sh)
    - [Gray-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/scripts/run_time_aca_graybox.sh)
    - [Black-box](https://github.com/ruiyang00/aca_dlis_review/tree/master/scripts/run_time_aca_blackbox.sh)
    - [Legitmate workload](https://github.com/ruiyang00/aca_dlis_review/tree/master/scripts/run_time_aca_legit.sh)
- [Time ACA on APEX](https://github.com/ruiyang00/aca_dlis_review/tree/master/apex): This directry contains time ACA on APEX source code and [script](https://github.com/ruiyang00/aca_dlis_review/blob/master/apex/run_time_aca.sh) to reproduce figure 8 results.
- [Experiment Scripts](https://github.com/ruiyang00/aca_dlis_review/tree/master/scripts): This directry contains the attack experiment scripts as well as the script to generate graybox sampled dataset for space ACA on data nodes and time ACA with different paramaters.
- [ALEX](https://github.com/ruiyang00/aca_dlis_review/tree/master/src/core): ALEX's source code

# 3. Dependencies for Attacks
- Space ACA on data nodes: [OR-Tools](https://developers.google.com/optimization/install)
- Space ACA on internal nodes: No dependencies
- Time ACA on ALEX: Python3 [KDE](https://scikit-learn.org/stable/install.html)
- Time ACA on APEX: [Persistent Memory Emulator](https://pmem.io/blog/2016/02/how-to-emulate-persistent-memory/). For this attack, we need use DRAM to emulate persistent memory by reconfigring kernel settings

# 3. Datasets
Datasets (Longitudes, Longlat, Lognormal, YCSB) used in this paper could be found in [ALEX](https://github.com/microsoft/ALEX) repo.


