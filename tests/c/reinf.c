/*
    Copyright 2022 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
    Miniature Mobile Robots group, Switzerland
    Author: Yves Piguet

    Licensed under the 3-Clause BSD License;
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    https://opensource.org/licenses/BSD-3-Clause
*/

// test of nn implementation: learning of obstacle avoidance with reinforcement

#include "nn/nn.h"
#include "nn/nn-alloc.h"
#include <stdio.h>

static void copyVec(NNFloat *dest, NNFloat const *src, int n) {
	for (int i = 0; i < n; i++) {
		dest[i] = src[i];
	}
}

static void printVec(NNFloat const *v, int n) {
	printf("[");
	for (int i = 0; i < n; i++) {
		printf("%s%.0f", i > 0 ? "," : "", v[i]);
	}
	printf("]");
}

static void reinf(NN *nn, NNFloat const *input, NNFloat const *output, NNFloat alpha) {
	NNFloat *nnInput = NNGetInputPtr(nn);
	NNFloat *nnOutput = NNGetOutputPtr(nn);

	printVec(input, nn->inputCount);
	printf(" := ");
	printVec(output, nn->outputCount);
	printf("\n");

	copyVec(nnInput, input, nn->inputCount);
	copyVec(nnOutput, output, nn->outputCount);
	NNHebbianRuleStep(nn, 0, alpha);
}

static void validate(NN *nn, NNFloat const *input) {
	NNFloat *nnInput = NNGetInputPtr(nn);
	NNFloat *nnOutput = NNGetOutputPtr(nn);

	copyVec(nnInput, input, nn->inputCount);
	NNEval(nn, NULL);

	printVec(input, nn->inputCount);
	printf(" -> ");
	printVec(nnOutput, nn->outputCount);
	printf("\n");
}

int main() {
	NN nn = { 0, 0, 0, 0, 0 };   // empty
	int nnSize[] = {2, 2};
	int nnLayerCount = 1;
	NNFloat alpha = 1e-6;
		// rationale: trial and error to get the result we want...

	printf("Number of layers: %d\n", nnLayerCount);
	printf("Size: ");
	for (int k = 0; k <= nnLayerCount; k++) {
		printf("%s%d", k > 0 ? ", " : "", nnSize[k]);
	}
	printf("\n");

	NNReset(&nn, nnLayerCount);
	for (int k = 0; k < nnLayerCount; k++) {
		NNAddLayer(&nn, nnSize[k], nnSize[k + 1], NNActivationIdentity);
	}

	NNObservations obs;
	NNFloat *input, *output;
	int status = NNObservationsInit(&obs, nn.inputCount, nn.outputCount, 10);
	// no obstacle: fast forward
	// 0, 0 -> 100, 100
	NNObservationGetPtr(&obs, obs.count++, &input, &output);
	input[0] = 0;
	input[1] = 0;
	output[0] = 100;
	output[1] = 100;
	// obstacle on left: turn right
	// 500, 0 -> 80, 20
	NNObservationGetPtr(&obs, obs.count++, &input, &output);
	input[0] = 500;
	input[1] = 0;
	output[0] = 80;
	output[1] = 20;
	// obstacle on right: turn left
	// 0, 500 -> 20, 80
	NNObservationGetPtr(&obs, obs.count++, &input, &output);
	input[0] = 0;
	input[1] = 500;
	output[0] = 20;
	output[1] = 80;
	// obstacle on both sides: stop
	// 500, 500 -> 0, 0
	NNObservationGetPtr(&obs, obs.count++, &input, &output);
	input[0] = 500;
	input[1] = 500;
	output[0] = 0;
	output[1] = 0;

	// learn with all observations
	NNClearWeights(&nn);
	for (int i = 0; i < obs.count; i++) {
		NNObservationGetPtr(&obs, i, &input, &output);
		reinf(&nn, input, output, alpha);
	}

	// validate with all observations
	for (int i = 0; i < obs.count; i++) {
		NNObservationGetPtr(&obs, i, &input, &output);
		validate(&nn, input);
	}
}
