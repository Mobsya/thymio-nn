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
#include <stdlib.h>
#include <math.h>

// uniform pseudorandom number between - and + amplitude
static NNFloat prand(NNFloat amplitude) {
#if defined(__APPLE__)
	return (int32_t)(arc4random() ^ 0x80000000) * (amplitude / 0x80000000);
#elif defined(__linux__)
	return (random() - 0x40000000l) * (amplitude / 0x40000000);
#else
	return (rand() - RAND_MAX / 2) * (amplitude / (RAND_MAX / 2));
#endif
}

static void copyFloats(NNFloat *dest, NNFloat const *src, int n) {
	for (int i = 0; i < n; i++) {
		dest[i] = src[i];
	}
}

static void resetFloats(NNFloat *dest, int n) {
	for (int i = 0; i < n; i++) {
		dest[i] = 0;
	}
}

static void accumulateFloats(NNFloat *dest, NNFloat *src, int n, NNFloat weight) {
	for (int i = 0; i < n; i++) {
		dest[i] += weight * src[i];
	}
}

static NNFloat tanhder(NNFloat p) {
	NNFloat tanhp = tanh(p);
	return 1 - tanhp * tanhp;
}

static NNFloat sigmoidder(NNFloat p) {
	NNFloat sigmoid = (1 + tanh(p / 2)) / 2;
	return sigmoid * (1 - sigmoid);
}

NNFloat *NNGetInputPtr(NN const *nn) {
	NNLayer *layerFirst = &nn->layer[0];
	return layerFirst->input;
}

NNFloat *NNGetOutputPtr(NN const *nn) {
	NNLayer *layerLast = &nn->layer[nn->layerCount - 1];
	return layerLast->output;
}

void NNClearWeights(NN *nn) {
	for (int k = 0; k < nn->layerCount; k++) {
		for (int i = 0; i < nn->layer[k].inputCount * nn->layer[k].outputCount; i++) {
			nn->layer[k].W[i] = 0;
		}
		for (int i = 0; i < nn->layer[k].outputCount; i++) {
			nn->layer[k].B[i] = 0;
		}
	}
}

void NNInitWeights(NN *nn) {
	for (int k = 0; k < nn->layerCount; k++) {
		NNFloat amplitude = 1 / sqrt(nn->layer[k].inputCount);
		for (int i = 0; i < nn->layer[k].inputCount * nn->layer[k].outputCount; i++) {
			nn->layer[k].W[i] = prand(amplitude);
		}
		for (int i = 0; i < nn->layer[k].outputCount; i++) {
			nn->layer[k].B[i] = 0;
		}
	}
}

void NNEval(NN *nn, NNFloat **P) {
	for (int k = 0; k < nn->layerCount; k++) {
		NNLayer *layer = &nn->layer[k];
		NNFloat const *input = k == 0 ? layer->input : nn->layer[k - 1].output;
		for (int i = 0; i < layer->outputCount; i++) {
			NNFloat p = layer->B[i];
			for (int j = 0; j < layer->inputCount; j++) {
				p += layer->W[i * layer->inputCount + j] * input[j];
			}
			if (P) {
				P[k][i] = p;
			}
			switch (layer->activation) {
			case NNActivationTanh:
				p = tanh(p);
				break;
			case NNActivationSigmoid:
				p = (1 + tanh(p / 2)) / 2;
				break;
			case NNActivationIdentity:
			default:
				break;
			}
			layer->output[i] = p;
		}
	}
}

void NNHebbianRuleStep(NN *nn, int layerIndex, NNFloat alpha) {
	NNLayer *layer = &nn->layer[layerIndex];
	for (int i = 0; i < layer->outputCount; i++) {
		for (int j = 0; j < layer->inputCount; j++) {
			layer->W[i * layer->inputCount + j]
				+= alpha * layer->input[j] * layer->output[i];
		}
	}
}

NNFloat NNBackPropCost(NN *nn, NNFloat const *output) {
	NNLayer *layer = &nn->layer[nn->layerCount - 1];
	NNFloat sumErr2 = 0;
	for (int i = 0; i < layer->outputCount; i++) {
		NNFloat err = output[i] - layer->output[i];
		sumErr2 += err * err;
	}
	return sumErr2 / 2;
}

int NNBackPropTempMemorySize(NN *nn) {
	int maxOutputCount = 0;
	int dataSize = 0;
	for (int k = 0; k < nn->layerCount; k++) {
		if (nn->layer[k].outputCount > maxOutputCount) {
			maxOutputCount = nn->layer[k].outputCount;
		}
		dataSize += nn->layer[k].outputCount * (2 + nn->layer[k].inputCount);
	}
	dataSize += maxOutputCount;

	return 3 * nn->layerCount * sizeof(NNFloat *) + dataSize * sizeof(NNFloat);
}

int NNBackPropInit(NN *nn, NNBackProp *bp, void *tempMem) {
	// E: one vector of size max(outputCount)
	// P: one vector of size outputCount per layer
	// Wg: one matrix of size outputCount-by-inputCount (same as W) per layer
	// Bg: one vector of size outputCount per layer
	int maxOutputCount = 0;
	int dataSize = 0;
	for (int k = 0; k < nn->layerCount; k++) {
		if (nn->layer[k].outputCount > maxOutputCount) {
			maxOutputCount = nn->layer[k].outputCount;
		}
		dataSize += nn->layer[k].outputCount * (2 + nn->layer[k].inputCount);
	}
	dataSize += maxOutputCount;

	bp->ptr = (NNFloat **)tempMem;
	bp->data = (NNFloat *)(bp->ptr + 3 * nn->layerCount);

	bp->E = bp->data;
	bp->P = bp->ptr;
	bp->Bg = bp->ptr + nn->layerCount;
	bp->Wg = bp->ptr + 2 * nn->layerCount;
	int offset = maxOutputCount;
	for (int k = 0; k < nn->layerCount; k++) {
		bp->P[k] = &bp->data[offset];
		offset += nn->layer[k].outputCount;
		bp->Bg[k] = &bp->data[offset];
		offset += nn->layer[k].outputCount;
		bp->Wg[k] = &bp->data[offset];
		offset += nn->layer[k].outputCount * nn->layer[k].inputCount;
	}

	NNBackPropResetGradients(nn, bp);

	return 1;
}

void NNBackPropResetGradients(NN *nn, NNBackProp *bp) {
	for (int k = 0; k < nn->layerCount; k++) {
		resetFloats(bp->Bg[k], nn->layer[k].outputCount);
		resetFloats(bp->Wg[k], nn->layer[k].outputCount * nn->layer[k].inputCount);
	}
}

void NNBackPropAddGradients(NN *nn, NNBackProp *bp) {
	// save output
	copyFloats(bp->E, nn->layer[nn->layerCount - 1].output,
		nn->layer[nn->layerCount - 1].outputCount);

	// feedforward
	NNEval(nn, bp->P);

	// error of last layer (column vector)
	// beware of the sign: E = Yobs - Y, W and B should be corrected by
	// adding gradients with a positive gain eta
	accumulateFloats(bp->E, nn->layer[nn->layerCount - 1].output,
		nn->layer[nn->layerCount - 1].outputCount, -1);

	// backprop
	for (int k = nn->layerCount - 1; k >= 0; k--) {
		NNLayer *layer = &nn->layer[k];

		// Bg = E .* phi'(P)
		// (column vector, length outputCount)
		switch (layer->activation) {
		case NNActivationTanh:
			for (int i = 0; i < layer->outputCount; i++) {
				bp->Bg[k][i] = bp->E[i] * tanhder(bp->P[k][i]);
			}
			break;
		case NNActivationSigmoid:
			for (int i = 0; i < layer->outputCount; i++) {
				bp->Bg[k][i] = bp->E[i] * sigmoidder(bp->P[k][i]);
			}
			break;
		case NNActivationIdentity:
		default:
			// phi' = 1
			for (int i = 0; i < layer->outputCount; i++) {
				bp->Bg[k][i] = bp->E[i];
			}
			break;
		}

		// Wg = Bg * U'
		// (matrix, same size as W: outputCount rows, inputCount columns)
		NNFloat const *input = k == 0 ? layer->input : nn->layer[k - 1].output;
		for (int i = 0; i < layer->outputCount; i++) {
			for (int j = 0; j < layer->inputCount; j++) {
				bp->Wg[k][i * layer->inputCount + j] = bp->Bg[k][i] * input[j];
			}
		}

		if (k > 0) {
			// E := W' * Bg
			// (column vector, length inputCount = outputCount of previous layer)
			for (int i = 0; i < layer->inputCount; i++) {
				bp->E[i] = 0;
				for (int j = 0; j < layer->outputCount; j++) {
					bp->E[i] += layer->W[j * layer->inputCount + i] * bp->Bg[k][j];
				}
			}
		}
	}
}

void NNBackPropApply(NN *nn, NNBackProp *bp, NNFloat eta) {
	for (int k = 0; k < nn->layerCount; k++) {
		accumulateFloats(nn->layer[k].B, bp->Bg[k],
			nn->layer[k].outputCount, eta);
		accumulateFloats(nn->layer[k].W, bp->Wg[k],
			nn->layer[k].outputCount * nn->layer[k].inputCount, eta);
	}
}

void NNObservationGetPtr(NNObservations *obs, int i,
	NNFloat **input, NNFloat **output)
{
	if (i > obs->maxCount) {
		*input = 0;
		*output = 0;
	} else {
		*input = &obs->data[i * (obs->inputCount + obs->outputCount)];
		*output = *input + obs->inputCount;
	}
}
