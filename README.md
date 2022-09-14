# Neural network extensions for the Thymio firmware

## Neural network

The implementation of a platform-independent neural network is in files `nn.h` and `nn.c`.

Data structure allocation depends on the platform. For a fixed-size network, it could be static. File `nn-alloc.h` declares generic functions; file `nn-alloc-stdlib.c` implements them using `malloc` and `free`.

The implementation can be tested with `tests/xor.c`, a stand-alone program which learns the exclusive-or function. The program is built by `Makefile`.

## Native functions for Aseba

Neural network functions are made available to the virtual machine of Aseba and the Aseba programming language as native functions. They need support for floating-point (arithmetic operators and function `tanh`). Native functions are implemented in C in files `thymio/nn-native.h`, `thymio/nn-native.c` and `thymio/nn-descriptions.c`.

## Test program for Aseba compiler and VM

Program `vmshell` contains the Aseba VM, the Aseba language compiler and the neural network functions. It meakes the development and tests of the new functions easier.

In addition to the neural network functions, `vmshell` implements a native function `test.display(a)` which displays its argument:

```
./vmshell --code 'call test.display([1,2,345])'
```

To compile `vmshell` with the makefile, copy or create a symbolic link `aseba` in the directory of `thymio-nn`, at the same level as this README file. The content or target should be the directory `aseba/aseba` from [https://github.com/Mobsya/aseba](https://github.com/Mobsya/aseba).
