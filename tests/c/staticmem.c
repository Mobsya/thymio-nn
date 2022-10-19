/*
    Copyright 2022 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
    Miniature Mobile Robots group, Switzerland
    Author: Yves Piguet

    Licensed under the 3-Clause BSD License;
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    https://opensource.org/licenses/BSD-3-Clause
*/

// test of staticalloc implementation

#include "nn/staticalloc.h"
#include <stdio.h>
#include <stdlib.h>

#define NSIMULTANEOUS 20
#define MAXSIZE 200
#define ITER 10000000

int main() {
	char *p[NSIMULTANEOUS];

	for (int i = 0; i < NSIMULTANEOUS; i++) {
		p[i] = NULL;
	}

	// malloc and free in a loop
	for (int i = 0; i < ITER; i++) {
		// large
		if (i % 13 == 0) {
			char *q = static_malloc(0x7fffffff);
			if (q != NULL) {
				fprintf(stderr, "Failure: i=%d, size=%d\n", i, 0x7fffffff);
				exit(1);
			}
		}

		if (p[i % NSIMULTANEOUS] != NULL) {
			static_free(p[i % NSIMULTANEOUS]);
		}
		int size = 1 + (7 * i) % (MAXSIZE - 1);
		p[i % NSIMULTANEOUS] = static_malloc(size);
		if (p[i % NSIMULTANEOUS] == NULL) {
			fprintf(stderr, "Failure: i=%d, size=%d\n", i, size);
			exit(1);
		}
	}

	return 0;
}
