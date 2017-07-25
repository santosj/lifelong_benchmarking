# lifelong_benhcmarking

This package provides a tool to easily benchmark lifelong strategies for mobile robots, more specifically spatio-temporal exploration strategies. This tool makes use of the MORSE simulation environment to create a dynamic and realistic tool based on real data acquired over 5 business days.

## Start the system

To run the system  run the following command:
```rosrun lifelong_benchmarking sim_start.sh  ```

This command starts tmux with all the tabs already pre-configured to run the dynamic simulation.

### Prerequisites

To use this tool, the following requirements must be met:
 * Ubuntu 14.04
 * ROS Indigo
 * [STRANDS](http://strands.acin.tuwien.ac.at/software.html) - The STRANDS system
 * [STRANDS MORSE] (https://github.com/santosj/strands_morse) - STRANDS MORSE with some variations


### Installing

Simply follow the instructions to install the STRANDS system, clone the STRANDS MORSE package and ```catkin_make```.


### Development

This package was developed to benchmark lifelong exploration strategies. However, it can be easily adapted in order to validate other lifelong decision-making strategies. At the moment is only possible to run 1 week long experiments, but new data is being process and prepared to be released as well as enhancements to the code in order to make this tool as generic as possible.
