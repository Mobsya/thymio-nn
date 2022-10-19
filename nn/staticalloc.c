/*
	Copyright 2022 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
	Miniature Mobile Robots group, Switzerland
	Author: Yves Piguet

	Licensed under the 3-Clause BSD License;
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at
	https://opensource.org/licenses/BSD-3-Clause
*/

#include "staticalloc.h"

/*
Simple implementation of malloc and free in a static block of memory of size STATICALLOC.
int mem[] contains compact blocks. Block at int *p is free or allocated. Free blocks
have p[0] = -size, allocated blocks have p[0] = +size; both kinds are followed by
size ints.

STATICALLOC should be set to the size in bytes to allocate in a static array.
*/

#if !defined(STATICALLOC)
#	define STATICALLOC 20000
#endif

#define TOTALSIZEINT (int)(STATICALLOC / sizeof(int))

static int mem[TOTALSIZEINT];
static int memInitialized = 0;

void *static_malloc(int n) {
	if (!memInitialized) {
		// initialization the first time malloc is called
		mem[0] = 1 - TOTALSIZEINT;
		memInitialized = 1;
	}

	// find a free block large enough
	for (int i = 0;
		i < TOTALSIZEINT;
		i += 1 + (mem[i] < 0 ? -mem[i] : mem[i])) {
		// merge free blocks
		while (mem[i] < 0
			&& i + 1 - mem[i] < TOTALSIZEINT
			&& mem[i + 1 - mem[i]] < 0) {
			mem[i] += mem[i + 1 - mem[i]] - 1;
		}

		// allocate if possible
		if (mem[i] < 0 && -mem[i] * sizeof(int) >= n) {
			// free block large enough
			void *p = (void *)&mem[i + 1];
			if (2 - mem[i] < n * sizeof(int)) {
				// no room for another free block
				mem[i] = -mem[i];
			} else {
				// room for another free block
				int ni = (n + sizeof(int) - 1) / sizeof(int);
				mem[i + 1 + ni] = -(-mem[i] - 1 - ni);
				mem[i] = ni;
			}
			return p;
		}
	}

	// no free block is large enough
	return NULL;
}

void static_free(void *p) {
	if (!p) {
		return;
	}

	// index in mem of the size before the allocated block
	int i = (int *)p - 1 - mem;

	// deallocate block
	mem[i] = -mem[i];
}
