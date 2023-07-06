# 1. Introduction

This repository contains the source code of our space ACA on data nodes, space ACA on internal nodes and time ACAs.

# 2. Contents

- [Space ACA on data nodes](https://github.com/ruiyang00/aca_dlis_review/tree/master/attack): This directry contains data node attack logic (MCK implementation).
- [Space ACA on internal nodes](https://github.com/ruiyang00/aca_dlis_review/tree/master/src/benchmark): The source code for space ACA on internal nodes.
- [Time ACA on ALEX](https://github.com/ruiyang00/aca_dlis_review/tree/master/src/benchmark): The source code for time ACA on ALEX.
- [Time ACA on APEX](https://github.com/ruiyang00/aca_dlis_review/tree/master/apex): This directry contains time ACA on APEX source code and [script](https://github.com/ruiyang00/aca_dlis_review/blob/master/apex/run_time_aca.sh) to reproduce figure 8 results. 
- [Experiment Scripts](https://github.com/ruiyang00/aca_dlis_review/tree/master/scripts): This directry contains the attack experiment scripts as well as the script to generate graybox sampled dataset for space ACA on data nodes and time ACA with different paramaters.
- [ALEX](https://github.com/ruiyang00/aca_dlis_review/tree/master/src/core): ALEX's source code

# 3. Dependencies for the attacks.
- Space ACA on data nodes: [OR-Tools]([https://developers.google.com/optimization](https://developers.google.com/optimization/install).
- Space ACA on internal nodes: No dependencies.
- Time ACA on ALEX: Python3 [KDE](https://scikit-learn.org/stable/install.html).
- Time ACA on APEX: [Persistent Memory Emulator](https://pmem.io/blog/2016/02/how-to-emulate-persistent-memory/). For this attack, we need use DRAM to emulate persistent memory by reconfigring kernel settings. 
# 4. Reproduce figures in our paper.
