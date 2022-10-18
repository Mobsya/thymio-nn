/*
	Copyright 2022 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
	Miniature Mobile Robots group, Switzerland
	Author: Yves Piguet

	Licensed under the 3-Clause BSD License;
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at
	https://opensource.org/licenses/BSD-3-Clause
*/

#include "nn.h"
#include "nn-alloc.h"

#if defined(STATICALLOC)

/*
Simple implementation of malloc and free in a static block of memory of size STATICALLOC.
int mem[] contains compact blocks. Block at int *p is free or allocated. Free blocks
have p[0] = -size, allocated blocks have p[0] = +size; both kinds are followed by
size ints.
*/

#define TOTALSIZEINT (int)(STATICALLOC / sizeof(int))

static int mem[TOTALSIZEINT];
static int memInitialized = 0;

#if !defined(NULL)
#	define NULL 0
#endif

#define malloc static_malloc
#define free static_free

static void *static_malloc(int n) {
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
		if (mem[i] < 0 && mem[i] * sizeof(int) >= n) {
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
printf("malloc -> %x\n", (int)p);
			return p;
		}
	}

	// no free block is large enough
	return NULL;
}

static void static_free(void *p) {
	if (!p) {
		return;
	}

	// index in mem of the size before the allocated block
	int i = (int *)p - 1 - mem;

	// deallocate block
printf("free %x\n", (int)p);
	mem[i] = -mem[i];
}

#else
#	include <stdlib.h>
#endif

// reset nn to empty, returning 1 for success or 0 for failure
int NNReset(NN *nn, int maxLayerCount) {
	// dealloc all layers
	for (int k = 0; k < nn->layerCount; k++) {
		if (nn->layer[k].data) {
			free(nn->layer[k].data);
		}
	}

	if (nn->layer) {
		free(nn->layer);
		nn->layer = 0;
	}

	nn->maxLayerCount = 0;
	nn->layerCount = 0;
	nn->inputCount = 0;
	nn->outputCount = 0;

	if (maxLayerCount > 0) {
		nn->layer = (NNLayer *)malloc(maxLayerCount * sizeof(NNLayer));
		if (!nn->layer) {
			return 0;
		}
		nn->maxLayerCount = maxLayerCount;
	}

	return 1;
}

// add a layer, returning 1 for success or 0 for failure
int NNAddLayer(NN *nn,
	int inputCount, int outputCount, NNActivation activation) {
	if (nn->layerCount >= nn->maxLayerCount
		|| inputCount <= 0 || outputCount <= 0) {
		return 0;
	}

	int dataCount = inputCount * outputCount // w
		+ outputCount   // b
		+ (nn->layerCount == 0 ? inputCount : 0) // input
		+ outputCount;  // output
	nn->layer[nn->layerCount].data = (NNFloat *)malloc(dataCount * sizeof(NNFloat));
	if (!nn->layer[nn->layerCount].data) {
		return 0;
	}

	nn->layer[nn->layerCount].W = nn->layer[nn->layerCount].data;
	nn->layer[nn->layerCount].B = nn->layer[nn->layerCount].W + inputCount * outputCount;
	if (nn->layerCount == 0) {
		nn->layer[nn->layerCount].input = nn->layer[nn->layerCount].B + outputCount;
		nn->layer[nn->layerCount].output = nn->layer[nn->layerCount].input + inputCount;
	} else {
		nn->layer[nn->layerCount].input = NULL;
		nn->layer[nn->layerCount].output = nn->layer[nn->layerCount].B + outputCount;
	}

	nn->layer[nn->layerCount].inputCount = inputCount;
	nn->layer[nn->layerCount].outputCount = outputCount;
	nn->layer[nn->layerCount].activation = activation;
	if (nn->layerCount == 0) {
		nn->inputCount = inputCount;
	}
	nn->outputCount = outputCount;
	nn->layerCount++;
	return 1;
}

int NNBackPropAllocStorage(NN *nn, void **backpropTempMem) {
	if (*backpropTempMem) {
		free((void *)*backpropTempMem);
		*backpropTempMem = NULL;
	}
	if (nn) {
		int size = NNBackPropTempMemorySize(nn);
		*backpropTempMem = (NNFloat *)malloc(size);
		if (!*backpropTempMem)
			return 0;
	}
	return 1;
}

int NNObservationsInit(NNObservations *obs, int inputCount, int outputCount,
	int maxObsCount) {
	if (obs->data) {
		free((void *)obs->data);
		obs->data = NULL;
		obs->maxCount = 0;
	}
	if (outputCount > 0) {
		obs->data = malloc((inputCount + outputCount) * maxObsCount * sizeof(NNFloat));
		if (!obs->data) {
			return 0;
		}
		obs->maxCount = maxObsCount;
		obs->count = 0;
		obs->inputCount = inputCount;
		obs->outputCount = outputCount;
	}
	return 1;
}
