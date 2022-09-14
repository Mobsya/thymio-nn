/*
	Copyright 2022 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
	Miniature Mobile Robots group, Switzerland
	Author: Yves Piguet

	Licensed under the 3-Clause BSD License;
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at
	https://opensource.org/licenses/BSD-3-Clause
*/

#ifndef __NNAlloc_H
#define __NNAlloc_H

#if defined(__cplusplus)
extern "C" {
#endif

// reset nn to empty, returning 1 for success or 0 for failure
int NNReset(NN *nn, int maxLayerCount);

// add a layer, returning 1 for success or 0 for failure
int NNAddLayer(NN *nn,
	int inputCount, int outputCount, NNActivation activation);

// alloc temporary storage for back propagation, or deallocate if nn is NULL
int NNBackPropAllocStorage(NN *nn, void **backpropTempMem);

#if defined(__cplusplus)
}
#endif

#endif
