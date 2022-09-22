/*
	Copyright 2022 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
	Miniature Mobile Robots group, Switzerland
	Author: Yves Piguet

	Licensed under the 3-Clause BSD License;
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at
	https://opensource.org/licenses/BSD-3-Clause
*/

#include "nn-natives.h"
#include "common/consts.h"

#include "nn/nn.h"
#include "nn/nn-alloc.h"
#include <math.h>

// single neural network

static NN nn = { 0, 0, 0, 0, 0 };   // empty
static NNBackProp bp = { 0, 0, 0, 0, 0, 0 };	// empty
static NNObservations obs = { 0, 0, 0, 0, 0 };	// empty
static void *backpropTempMem = 0;
static enum {
	NNErrorOk = 0,
	NNErrorOutOfMemory,
	NNErrorNoNN,
	NNErrorIndexOutOfRange,
	NNErrorUnsuitableForHebbianRule,
	NNErrorDatasetSizeExceeded
} error = 0;

static void fractionApprox(NNFloat x, int16_t *num, int16_t *den) {

	const int max = 0x7fff;

	// Farey sequence starting around x (a/b <= x <= c/d)
	int a = (int)floor(x), b = 1, c = a + 1, d = 1;

	// special case for int
	if (x == (NNFloat)a) {
		*num = a;
		*den = 1;
		return;
	}

	// approx
	while (b <= max && d <= max) {
		NNFloat m = (NNFloat)(a + c) / (b + d);
		if (x == m) {
			if (b + d <= max) {
				*num = a + c;
				*den = b + d;
				return;
			} else {
				break;
			}
		} else if (x > m) {
			a += c;
			b += d;
		} else {
			c += a;
			d += b;
		}
	}

	if (b > max) {
		*num = c;
		*den = d;
	} else {
		*num = a;
		*den = b;
	}
}

// nn.geterror(e)
void NN_nngeterror(AsebaVMState *vm) {
	int16_t *e = &vm->variables[AsebaNativePopArg(vm)];
	*e = (int16_t)error;
}

// nn.reseterror()
void NN_nnreseterror(AsebaVMState *vm) {
	error = NNErrorOk;
}

// nn.init(inputCount, [outputCount1, outputCount2, ...], [activationCode1, activationCode2, ...])
void NN_nninit(AsebaVMState *vm) {
	uint16_t const inputCount = vm->variables[AsebaNativePopArg(vm)];
	uint16_t const outputCountAddr = AsebaNativePopArg(vm);
	uint16_t const activationCodeAddr = AsebaNativePopArg(vm);
	uint16_t const layerCount = AsebaNativePopArg(vm);

	if (!NNReset(&nn, layerCount)) {
		error = NNErrorOutOfMemory;
		return;
	}
	for (int i = 0; i < layerCount; i++) {
		if (!NNAddLayer(&nn,
			i == 0 ? inputCount : vm->variables[outputCountAddr + i - 1],
			vm->variables[outputCountAddr + i],
			vm->variables[activationCodeAddr + i] == 1 ? NNActivationTanh
				: vm->variables[activationCodeAddr + i] == 2 ? NNActivationSigmoid
				: NNActivationNoop)) {
			error = NNErrorOutOfMemory;
			return;
		}
	}

	NNInitWeights(&nn);
}

void NN_nnreset(AsebaVMState *vm) {
	NNInitWeights(&nn);
}

void NN_nnclear(AsebaVMState *vm) {
	NNClearWeights(&nn);
}

// nn.getweight(layerIndex, inputIndex, outputIndex, num, den)
void NN_nngetweight(AsebaVMState *vm) {
	const int16_t layerIndex = vm->variables[AsebaNativePopArg(vm)];
	const int16_t inputIndex = vm->variables[AsebaNativePopArg(vm)];
	const int16_t outputIndex = vm->variables[AsebaNativePopArg(vm)];
	int16_t *num = &vm->variables[AsebaNativePopArg(vm)];
	int16_t *den = &vm->variables[AsebaNativePopArg(vm)];

	if (nn.layerCount == 0) {
		error = NNErrorNoNN;
	} else if (layerIndex >= 0 && layerIndex < nn.layerCount
		&& inputIndex >= 0 && inputIndex < nn.layer[layerIndex].inputCount
		&& outputIndex >= 0 && outputIndex < nn.layer[layerIndex].outputCount) {
			NNLayer *layer = &nn.layer[layerIndex];
			fractionApprox(layer->W[outputIndex * layer->inputCount + inputIndex],
				num, den);
	} else {
		error = NNErrorIndexOutOfRange;
	}
}

// nn.setweight(layerIndex, inputIndex, outputIndex, num, den)
void NN_nnsetweight(AsebaVMState *vm) {
	const int16_t layerIndex = vm->variables[AsebaNativePopArg(vm)];
	const int16_t inputIndex = vm->variables[AsebaNativePopArg(vm)];
	const int16_t outputIndex = vm->variables[AsebaNativePopArg(vm)];
	const int16_t num = vm->variables[AsebaNativePopArg(vm)];
	const int16_t den = vm->variables[AsebaNativePopArg(vm)];

	if (nn.layerCount == 0) {
		error = NNErrorNoNN;
	} else if (layerIndex >= 0 && layerIndex < nn.layerCount
		&& inputIndex >= 0 && inputIndex < nn.layer[layerIndex].inputCount
		&& outputIndex >= 0 && outputIndex < nn.layer[layerIndex].outputCount
		&& den != 0) {
			NNLayer *layer = &nn.layer[layerIndex];
			layer->W[outputIndex * layer->inputCount + inputIndex] = (NNFloat)num / den;
	} else {
		error = NNErrorIndexOutOfRange;
	}
}

// nn.getweights(layerIndex, num, den)
void NN_nngetweights(AsebaVMState *vm) {
	const int16_t layerIndex = vm->variables[AsebaNativePopArg(vm)];
	int16_t *num = &vm->variables[AsebaNativePopArg(vm)];
	int16_t *den = &vm->variables[AsebaNativePopArg(vm)];
	uint16_t const length = AsebaNativePopArg(vm);

	if (nn.layerCount == 0) {
		error = NNErrorNoNN;
	} else if (layerIndex >= 0 && layerIndex < nn.layerCount) {
		NNLayer *layer = &nn.layer[layerIndex];
		for (int i = 0; i < length && i < layer->inputCount * layer->outputCount; i++) {
			fractionApprox(layer->W[i], &num[i], &den[i]);
		}
	} else {
		error = NNErrorIndexOutOfRange;
	}
}

// nn.setweights(layerIndex, num, den)
void NN_nnsetweights(AsebaVMState *vm) {
	const int16_t layerIndex = vm->variables[AsebaNativePopArg(vm)];
	int16_t *num = &vm->variables[AsebaNativePopArg(vm)];
	int16_t *den = &vm->variables[AsebaNativePopArg(vm)];
	uint16_t const length = AsebaNativePopArg(vm);

	if (nn.layerCount == 0) {
		error = NNErrorNoNN;
	} else if (layerIndex >= 0 && layerIndex < nn.layerCount) {
		NNLayer *layer = &nn.layer[layerIndex];
		for (int i = 0; i < length && i < layer->inputCount * layer->outputCount; i++) {
			layer->W[i] = (NNFloat)num[i] / den[i];
		}
	} else {
		error = NNErrorIndexOutOfRange;
	}
}

void NN_nnsetoffset(AsebaVMState *vm) {
	const int16_t layerIndex = vm->variables[AsebaNativePopArg(vm)];
	const int16_t index = vm->variables[AsebaNativePopArg(vm)];
	const int16_t num = vm->variables[AsebaNativePopArg(vm)];
	const int16_t den = vm->variables[AsebaNativePopArg(vm)];

	if (nn.layerCount == 0) {
		error = NNErrorNoNN;
	} else if (layerIndex >= 0 && layerIndex < nn.layerCount
		&& index >= 0 && index < nn.layer[layerIndex].outputCount
		&& den != 0) {
			NNLayer *layer = &nn.layer[layerIndex];
			layer->B[index] = (NNFloat)num / den;
	} else {
		error = NNErrorIndexOutOfRange;
	}
}

void NN_nngetoffset(AsebaVMState *vm) {
	const int16_t layerIndex = vm->variables[AsebaNativePopArg(vm)];
	const int16_t index = vm->variables[AsebaNativePopArg(vm)];
	int16_t *num = &vm->variables[AsebaNativePopArg(vm)];
	int16_t *den = &vm->variables[AsebaNativePopArg(vm)];

	if (nn.layerCount == 0) {
		error = NNErrorNoNN;
	} else if (layerIndex >= 0 && layerIndex < nn.layerCount
		&& index >= 0 && index < nn.layer[layerIndex].outputCount
		&& den != 0) {
			NNLayer *layer = &nn.layer[layerIndex];
			fractionApprox(layer->B[index], num, den);
	} else {
		error = NNErrorIndexOutOfRange;
	}
}

// nn.getoffsets(layerIndex, num, den)
void NN_nngetoffsets(AsebaVMState *vm) {
	const int16_t layerIndex = vm->variables[AsebaNativePopArg(vm)];
	int16_t *num = &vm->variables[AsebaNativePopArg(vm)];
	int16_t *den = &vm->variables[AsebaNativePopArg(vm)];
	uint16_t const length = AsebaNativePopArg(vm);

	if (nn.layerCount == 0) {
		error = NNErrorNoNN;
	} else if (layerIndex >= 0 && layerIndex < nn.layerCount) {
		NNLayer *layer = &nn.layer[layerIndex];
		for (int i = 0; i < length && i < layer->outputCount; i++) {
			fractionApprox(layer->B[i], &num[i], &den[i]);
		}
	} else {
		error = NNErrorIndexOutOfRange;
	}
}

// nn.setoffsets(layerIndex, num, den)
void NN_nnsetoffsets(AsebaVMState *vm) {
	const int16_t layerIndex = vm->variables[AsebaNativePopArg(vm)];
	int16_t *num = &vm->variables[AsebaNativePopArg(vm)];
	int16_t *den = &vm->variables[AsebaNativePopArg(vm)];
	uint16_t const length = AsebaNativePopArg(vm);

	if (nn.layerCount == 0) {
		error = NNErrorNoNN;
	} else if (layerIndex >= 0 && layerIndex < nn.layerCount) {
		NNLayer *layer = &nn.layer[layerIndex];
		for (int i = 0; i < length && i < layer->outputCount; i++) {
			layer->B[i] = (NNFloat)num[i] / den[i];
		}
	} else {
		error = NNErrorIndexOutOfRange;
	}
}

void NN_nngetinputs(AsebaVMState *vm) {
	int16_t *inputs = &vm->variables[AsebaNativePopArg(vm)];
	uint16_t const length = AsebaNativePopArg(vm);

	if (nn.layerCount > 0) {
		NNLayer *layer0 = &nn.layer[0];
		for (int i = 0; i < layer0->inputCount && i < length; i++) {
			inputs[i] = round(layer0->input[i]);
		}
	} else {
		error = NNErrorNoNN;
	}
}

void NN_nnsetinputs(AsebaVMState *vm) {
	int16_t *inputs = &vm->variables[AsebaNativePopArg(vm)];
	uint16_t const length = AsebaNativePopArg(vm);

	if (nn.layerCount > 0) {
		NNLayer *layer0 = &nn.layer[0];
		for (int i = 0; i < layer0->inputCount && i < length; i++) {
			layer0->input[i] = (NNFloat)inputs[i];
		}
	} else {
		error = NNErrorNoNN;
	}
}

void NN_nngetoutputs(AsebaVMState *vm) {
	int16_t *outputs = &vm->variables[AsebaNativePopArg(vm)];
	uint16_t const length = AsebaNativePopArg(vm);

	if (nn.layerCount > 0) {
		NNLayer *layerLast = &nn.layer[nn.layerCount - 1];
		for (int i = 0; i < layerLast->outputCount && i < length; i++) {
			outputs[i] = (int16_t)round(layerLast->output[i]);
		}
	} else {
		error = NNErrorNoNN;
	}
}

void NN_nnsetoutputs(AsebaVMState *vm) {
	int16_t *outputs = &vm->variables[AsebaNativePopArg(vm)];
	uint16_t const length = AsebaNativePopArg(vm);

	if (nn.layerCount > 0) {
		NNLayer *layerLast = &nn.layer[nn.layerCount - 1];
		for (int i = 0; i < layerLast->outputCount && i < length; i++) {
			layerLast->output[i] = (NNFloat)outputs[i];
		}
	} else {
		error = NNErrorNoNN;
	}
}

void NN_nneval(AsebaVMState *vm) {
	NNEval(&nn, NULL);
}

void NN_nnhebbianrule(AsebaVMState *vm) {
	if (nn.layerCount == 1 && nn.layer[0].activation == NNActivationNoop) {
		int16_t const alphanum = vm->variables[AsebaNativePopArg(vm)];
		int16_t const alphaden = vm->variables[AsebaNativePopArg(vm)];
		NNHebbianRuleStep(&nn, 0, (NNFloat)alphanum / alphaden);
	} else {
		error = NNErrorUnsuitableForHebbianRule;
	}
}

void NN_nnbackprop(AsebaVMState *vm) {
	if (nn.layerCount == 0) {
		error = NNErrorNoNN;
	} else if (!NNBackPropAllocStorage(&nn, &backpropTempMem)) {
		error = NNErrorOutOfMemory;
	} else {
		int16_t const etanum = vm->variables[AsebaNativePopArg(vm)];
		int16_t const etaden = vm->variables[AsebaNativePopArg(vm)];
		if (NNBackPropInit(&nn, &bp, backpropTempMem)) {
			NNBackPropAddGradients(&nn, &bp);
			NNBackPropApply(&nn, &bp, (NNFloat)etanum / etaden);
		}
	}
}

void NN_nndatasetinit(AsebaVMState *vm) {
	uint16_t const observationMaxCount = vm->variables[AsebaNativePopArg(vm)];
	NNObservationsInit(&obs, nn.inputCount, nn.outputCount, observationMaxCount);
}

void NN_nndatasetadd(AsebaVMState *vm) {
	int16_t *input = &vm->variables[AsebaNativePopArg(vm)];
	int16_t *output = &vm->variables[AsebaNativePopArg(vm)];
	uint16_t const inputLength = AsebaNativePopArg(vm);
	uint16_t const outputLength = AsebaNativePopArg(vm);

	if (nn.layerCount == 0) {
		error = NNErrorNoNN;
	} else if (obs.count >= obs.maxCount) {
		error = NNErrorDatasetSizeExceeded;
	} else {
		NNFloat *dataSetInput, *dataSetOutput;
		NNObservationGetPtr(&obs, obs.count, &dataSetInput, &dataSetOutput);
		for (int i = 0; i < inputLength && i < nn.inputCount; i++) {
			dataSetInput[i] = (NNFloat)input[i];
		}
		for (int i = 0; i < outputLength && i < nn.outputCount; i++) {
			dataSetOutput[i] = (NNFloat)output[i];
		}
		obs.count++;
	}
}

void NN_nnbackpropdataset(AsebaVMState *vm) {
	if (nn.layerCount == 0) {
		error = NNErrorNoNN;
	} else if (!NNBackPropAllocStorage(&nn, &backpropTempMem)) {
		error = NNErrorOutOfMemory;
	} else {
		int16_t const etanum = vm->variables[AsebaNativePopArg(vm)];
		int16_t const etaden = vm->variables[AsebaNativePopArg(vm)];
		NNFloat eta = (NNFloat)etanum / etaden;
		int16_t const numIter = vm->variables[AsebaNativePopArg(vm)];
		if (NNBackPropInit(&nn, &bp, backpropTempMem)) {
			for (int i = 0; i < numIter; i++) {
				for (int j = 0; j < obs.count; j++) {
					NNFloat *input, *output;
					NNObservationGetPtr(&obs, j, &input, &output);

					NNFloat *nnInput = NNGetInputPtr(&nn);
					NNFloat *nnOutput = NNGetOutputPtr(&nn);
					for (int k = 0; k < nn.inputCount; k++) {
						nnInput[k] = input[k];
					}
					for (int k = 0; k < nn.outputCount; k++) {
						nnOutput[k] = output[k];
					}

					NNBackPropResetGradients(&nn, &bp);
					NNBackPropAddGradients(&nn, &bp);
					NNBackPropApply(&nn, &bp, eta);
				}
			}
		}
	}
}
