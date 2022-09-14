/*
    Copyright 2022 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
    Miniature Mobile Robots group, Switzerland
    Author: Yves Piguet

    Licensed under the 3-Clause BSD License;
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    https://opensource.org/licenses/BSD-3-Clause
*/

// test of nn implementation: learning of xor function

#include "nn/nn.h"
#include "nn/nn-alloc.h"
#include <stdio.h>

static void step(NN *nn, NNBackProp *bp, NNFloat const *input, NNFloat const *output, NNFloat eta) {
	NNFloat *nnInput = NNGetInputPtr(nn);
	NNFloat *nnOutput = NNGetOutputPtr(nn);

	printf("[");
	for (int i = 0; i < nn->inputCount; i++) {
		nnInput[i] = input[i];
		printf("%s%.0f", i > 0 ? "," : "", input[i]);
	}
	NNEval(nn, NULL);
	printf("]:=[");
	for (int i = 0; i < nn->outputCount; i++) {
		printf("%s%.0f", i > 0 ? "," : "", output[i]);
	}
	printf("] -> [");
	for (int i = 0; i < nn->outputCount; i++) {
		printf("%s%.2f", i > 0 ? "," : "", nnOutput[i]);
		nnOutput[i] = output[i];
	}

	NNBackPropResetGradients(nn, bp);
	NNBackPropAddGradients(nn, bp);
	NNBackPropApply(nn, bp, eta);
	NNEval(nn, NULL);

	printf("]; bp -> [");
	for (int i = 0; i < nn->outputCount; i++) {
		printf("%s%.2f", i > 0 ? "," : "", nnOutput[i]);
	}
	printf("]\n");
}

int main() {
	NN nn = { 0, 0, 0, 0, 0 };   // empty
	NNBackProp bp = { 0, 0, 0, 0, 0, 0 };	// empty
	int nnSize[] = {2, 3, 1};
	NNActivation activation = NNActivationTanh;
	int nnLayerCount = sizeof(nnSize) / sizeof(int) - 1;
	int maxIter = 2000;
	void *backpropTempMem = 0;
	NNFloat eta = 0.02;

	printf("Number of layers: %d\n", nnLayerCount);
	printf("Size: ");
	for (int k = 0; k <= nnLayerCount; k++) {
		printf("%s%d", k > 0 ? ", " : "", nnSize[k]);
	}
	printf("\n");

	NNReset(&nn, nnLayerCount);
	for (int k = 0; k < nnLayerCount; k++) {
		NNAddLayer(&nn, nnSize[k], nnSize[k + 1], activation);
	}

	NNInitWeights(&nn);

	NNBackPropAllocStorage(&nn, &backpropTempMem);
	NNBackPropInit(&nn, &bp, backpropTempMem);

	NNFloat input[2], output[1];
	for (int i = 0; i < maxIter; i++) {
		// 0, 0 -> 0
		input[0] = 0;
		input[1] = 0;
		output[0] = 0;
		step(&nn, &bp, input, output, eta);
		// 0, 1 -> 1
		input[0] = 0;
		input[1] = 1;
		output[0] = 1;
		step(&nn, &bp, input, output, eta);
		// 1, 0 -> 1
		input[0] = 1;
		input[1] = 0;
		output[0] = 1;
		step(&nn, &bp, input, output, eta);
		// 1, 1 -> 0
		input[0] = 1;
		input[1] = 1;
		output[0] = 0;
		step(&nn, &bp, input, output, eta);
	}
}
