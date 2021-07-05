# Misra'83 token-based mutex
An implementation of a token-based mutual exclusion with token loss detection and recovery using a Misra'83 algorithm.

This program simulates an algorithm run on a distributed system with a user-defined number of nodes.
It uses [OpenMPI](https://www.open-mpi.org) to provision such a system and serve as a medium of communication in this system.

You can force a certain node to lose a particular token by inputting _q_ for _ping_ or _w_ for _ping_, followed by
a whitespace, followed by the number of the node (starting from 0) to STDIN and pressing ENTER.

For instance, input `q 0` to make Process 0 lose the next PING token, and input `w 2` to make Process 2 lose the next PONG token.

The algorithm should handle the loss of one token at a time.

## Build prerequisites
    CMake 3.9 (it will probably compile using older versions too, see the last paragraph)
    C++17 compliant compiler
    OpenMPI

## Build instructions
```
git clone https://github.com/kwencel/Misra83
cd Misra83
cmake .
make
```

## How to use
Invoke compiled executables by mpirun with at least 2 processes, for example:
```
mpirun -np 3 Misra83 | tee log && clear && echo "----- SORTED LOG -----" && cat log | sort
```
It will run the program and stream the logs as they are generated.
After the program completes however, it will display the log sorted by the messages
[lamport timestamp](https://en.wikipedia.org/wiki/Lamport_timestamp), which is crucial for analysing the program runtime.

## Older CMake version?
Try to change the minimum required version in CMakeLists.txt to match the version you have installed. There shouldn't be any issues.
