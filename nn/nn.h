/*
	Copyright 2022 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
	Miniature Mobile Robots group, Switzerland
	Author: Yves Piguet

	Licensed under the 3-Clause BSD License;
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at
	https://opensource.org/licenses/BSD-3-Clause
*/

#ifndef __NN_H
#define __NN_H

#if defined(__cplusplus)
extern "C" {
#endif

typedef float NNFloat;

typedef enum {
	NNActivationNoop = 0,
	NNActivationTanh,
	NNActivationSigmoid
} NNActivation;

typedef struct {
	int inputCount;
	int outputCount;
	NNActivation activation;
	NNFloat *data;  // block of data for w, b, input, output
	NNFloat *W; // W[i * inputCount + j] between input j and output i
	NNFloat *B;
	NNFloat *input; // or NULL for output of previous layer
	NNFloat *output;
} NNLayer;

typedef struct {
	int maxLayerCount;
	int layerCount;
	int inputCount;
	int outputCount;
	NNLayer *layer;
} NN;

typedef struct {
	NNFloat **ptr;	// block of pointers for E
	NNFloat *data;	// block of data for E
	NNFloat *E;	// E[i] = error for layer i
	NNFloat **P;	// P[i] = W[i] * y[i-1] + B[i] (i.e. output before activation)
	NNFloat **Wg;	// Wg[i] = weight gradient
	NNFloat **Bg;	// Bg[i] = offset gradient
} NNBackProp;

typedef struct {
	int maxObsCount;
	int obsCount;
	int inputCount;
	int outputCount;
	NNFloat *data;	// block of data for input and output data
} NNObservations;

// get address of nn inputs
NNFloat *NNGetInputPtr(NN const *nn);

// get address of nn outputs
NNFloat *NNGetOutputPtr(NN const *nn);

// initialize weights and offsets to 0
void NNClearWeights(NN *nn);

// initialize weights to pseudo-random values and offsets to 0
void NNInitWeights(NN *nn);

// evaluate output of each layer from first to last
// output before activation is stored in P[i] (if not NULL)
void NNEval(NN *nn, NNFloat **P);

// apply hebbian rule
void NNHebbianRuleStep(NN *nn, int layerIndex, NNFloat alpha);

// calculate cost function for back propagation after NNEval()
NNFloat NNBackPropCost(NN *nn, NNFloat const *output);

// calculate amount of temporary memory (in bytes) required for backprop
int NNBackPropTempMemorySize(NN *nn);

// initialize structures for backprop
int NNBackPropInit(NN *nn, NNBackProp *bp, void *tempMem);

// reset gradients for backprop
void NNBackPropResetGradients(NN *nn, NNBackProp *bp);

// add gradient based on 1st layer input and last layer output
void NNBackPropAddGradients(NN *nn, NNBackProp *bp);

// apply one step of back propagation using gradients obtained by
// NNResetBackProp and NNAddGradients
void NNBackPropApply(NN *nn, NNBackProp *bp, NNFloat eta);

// get address of input and output vectors of an observation
void NNObservationGetPtr(NNObservations *obs, int i,
	NNFloat **input, NNFloat **output);

#if defined(__cplusplus)
}
#endif

#endif
