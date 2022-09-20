/*
    Copyright 2022 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
    Miniature Mobile Robots group, Switzerland
    Author: Yves Piguet

    Licensed under the 3-Clause BSD License;
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    https://opensource.org/licenses/BSD-3-Clause
*/

#ifndef __NN_NATIVES_H
#define __NN_NATIVES_H

#include "vm/vm.h"
#include "vm/natives.h"

#if defined(__cplusplus)
extern "C" {
#endif

// configuration

void NN_nngeterror(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nngeterror;

void NN_nnreseterror(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nnreseterror;

void NN_nninit(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nninit;

void NN_nnreset(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nnreset;

void NN_nnclear(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nnclear;

void NN_nngetweight(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nngetweight;

void NN_nnsetweight(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nnsetweight;

void NN_nngetweights(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nngetweights;

void NN_nnsetweights(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nnsetweights;

void NN_nngetoffset(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nngetoffset;

void NN_nnsetoffset(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nnsetoffset;

void NN_nngetoffsets(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nngetoffsets;

void NN_nnsetoffsets(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nnsetoffsets;

void NN_nngetinputs(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nngetinputs;

void NN_nnsetinputs(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nnsetinputs;

void NN_nngetoutputs(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nngetoutputs;

void NN_nnsetoutputs(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nnsetoutputs;

void NN_nneval(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nneval;

void NN_nnhebbianrule(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nnhebbianrule;

void NN_nnbackprop(AsebaVMState *vm);
extern AsebaNativeFunctionDescription NNNativeDescription_nnbackprop;

// defines listing all native functions and their descriptions

#define NN_NATIVES_DESCRIPTIONS \
	&NNNativeDescription_nngeterror, \
	&NNNativeDescription_nnreseterror, \
    &NNNativeDescription_nninit, \
    &NNNativeDescription_nnreset, \
    &NNNativeDescription_nnclear, \
    &NNNativeDescription_nngetweight, \
    &NNNativeDescription_nnsetweight, \
    &NNNativeDescription_nngetweights, \
    &NNNativeDescription_nnsetweights, \
    &NNNativeDescription_nngetoffset, \
    &NNNativeDescription_nnsetoffset, \
    &NNNativeDescription_nngetoffsets, \
    &NNNativeDescription_nnsetoffsets, \
    &NNNativeDescription_nngetinputs, \
    &NNNativeDescription_nnsetinputs, \
    &NNNativeDescription_nngetoutputs, \
    &NNNativeDescription_nnsetoutputs, \
    &NNNativeDescription_nneval, \
	&NNNativeDescription_nnhebbianrule, \
	&NNNativeDescription_nnbackprop

#define NN_NATIVES_FUNCTIONS \
	NN_nngeterror, \
	NN_nnreseterror, \
    NN_nninit, \
    NN_nnreset, \
    NN_nnclear, \
    NN_nngetweight, \
    NN_nnsetweight, \
    NN_nngetweights, \
    NN_nnsetweights, \
    NN_nngetoffset, \
    NN_nnsetoffset, \
    NN_nngetoffsets, \
    NN_nnsetoffsets, \
    NN_nngetinputs, \
    NN_nnsetinputs, \
    NN_nngetoutputs, \
    NN_nnsetoutputs, \
    NN_nneval, \
	NN_nnhebbianrule, \
	NN_nnbackprop

#if defined(__cplusplus)
}
#endif

#endif
