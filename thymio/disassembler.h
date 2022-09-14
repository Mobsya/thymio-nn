/*
    Copyright 2017-2022 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
    Miniature Mobile Robots group, Switzerland
    Author: Yves Piguet

    Licensed under the 3-Clause BSD License;
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    https://opensource.org/licenses/BSD-3-Clause
*/

// Disassembler for Aseba bytecode

#include "vm/vm.h"
#include "vm/natives.h"
#include "common/productids.h"
#include "common/consts.h"

#ifdef __cplusplus

/**	Disassemble Aseba bytecode
	@param[in] bytecode bytecode, starting with number of events and event dispatch table
	@param[in] bytecodeSize size of bytecode
	@param[in] compiler compiler object used to resolve symbols, or NULL if not available
*/
#if defined(USE_COMPILER)
void disassembleWithCompiler(uint16_t const *bytecode, uint16_t bytecodeSize, Aseba::Compiler *compiler);
#else
void disassembleWithCompiler(uint16_t const *bytecode, uint16_t bytecodeSize, void *compiler);
#endif

extern "C" {
#endif

void disassemble(uint16_t const *bytecode, uint16_t bytecodeSize);

void compileAndDisassemble(AsebaVMState *vm, char const *source);

#ifdef __cplusplus
}
#endif
