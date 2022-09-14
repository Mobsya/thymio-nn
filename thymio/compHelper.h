/*
    Copyright 2017-2022 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
    Miniature Mobile Robots group, Switzerland
    Author: Yves Piguet

    Licensed under the 3-Clause BSD License;
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    https://opensource.org/licenses/BSD-3-Clause
*/

// Compiler helper functions

#include "vm/vm.h"
#include "vm/natives.h"
#include "common/productids.h"

#ifdef __cplusplus

#include "compiler/compiler.h"

std::wstring read_source(const std::string& filename);

/**	Compile code and store it into VM
	@param[in,out] vm Aseba VM
	@param[in] source Aseba source code
	@param[in,out] compiler compiler object
	@return 0 for success, not 0 for failure
*/
int compileWithCompiler(AsebaVMState *vm, char const *source, Aseba::Compiler &compiler);

extern "C" {
#endif

char const *readSource(char const *path);

/**	Compile code and store it into VM
	@param[in,out] vm Aseba VM
	@param[in] path path of Aseba source code file
	@return 0 for success, not 0 for failure
*/
int compile(AsebaVMState *vm, char const *path);

#ifdef __cplusplus
}
#endif
