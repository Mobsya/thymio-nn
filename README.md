# Neural network extensions for the Thymio firmware

The implementation of a platform-independent neural network is in files `nn.h` and `nn.c`.

Data structure allocation depends on the platform. For a fixed-size network, it could be static. File `nn-alloc.h` declares generic functions; file `nn-alloc-stdlib.c` implements them using `malloc` and `free`.

The implementation can be tested with `tests/xor.c`, a stand-alone program which learns the exclusive-or function. The program is built by `Makefile`.
