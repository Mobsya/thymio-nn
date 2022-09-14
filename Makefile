#   Copyright 2018-2021 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
#   Miniature Mobile Robots group, Switzerland
#   Author: Yves Piguet

#   Licensed under the 3-Clause BSD License;
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#   https://opensource.org/licenses/BSD-3-Clause


CFLAGS = -g -I.

vpath %.c nn:tests

test-nn-xor: nn.o nn-alloc-stdlib.o xor.o
	$(CC) -g -o $@ $^ -lm
