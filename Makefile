#   Copyright 2018-2021 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
#   Miniature Mobile Robots group, Switzerland
#   Author: Yves Piguet

#   Licensed under the 3-Clause BSD License;
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#   https://opensource.org/licenses/BSD-3-Clause


.PHONY: all
all: vmshell test-nn-backprop test-nn-xor

CFLAGS = -g -I. -Iaseba -Ithymio
CXXFLAGS = -g -I. -Iaseba

vpath %.c aseba/vm:aseba/transport/buffer:aseba/compiler:thymio:nn:tests/c
vpath %.cpp aseba/vm:aseba/compiler:aseba/common/utils:aseba/common/msg:thymio

vmobj = vm.o vm-buffer.o
vmnnobj = nn.o nn-alloc-stdlib.o nn-descriptions.o nn-natives.o
compobj = analysis.o compiler.o errors.o identifier-lookup.o lexer.o parser.o tree-build.o tree-dump.o tree-expand.o tree-emit.o tree-optimize.o tree-typecheck.o utils.o FormatableString.o TargetDescription.o

vmshell: vmshell.o compHelper.o disassembler.o $(vmobj) $(compobj) $(vmnnobj)
	$(CXX) -g -o $@ $^

disassembler.o: disassembler.cpp
	$(CXX) $(CXXFLAGS) -DUSE_COMPILER -c -o $@ $<

test-nn-backprop: nn.o nn-alloc-stdlib.o bp.o
	$(CC) -g -o $@ $^ -lm

test-nn-xor: nn.o nn-alloc-stdlib.o xor.o
	$(CC) -g -o $@ $^ -lm

.PHONY: tests
tests: test-nn-backprop
	./test-nn-backprop --eta 0.02 --input 2 --layer 3 tanh --layer 1 tanh --training tests/datasets/xor.csv --validation tests/datasets/xor.csv --iter 10000 --verbose
