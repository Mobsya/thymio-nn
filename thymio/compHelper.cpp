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

#include "compHelper.h"

#include "vm/vm.h"
#include "vm/natives.h"
#include "common/productids.h"
#include "common/utils/utils.h"
#include "common/consts.h"

using namespace Aseba;

#include <string>
#include <iostream>
#include <locale>
#include <fstream>
#include <sstream>
#include <valarray>

#include <string.h>

// should be declared in an include file
extern "C" const AsebaNativeFunctionDescription * const * AsebaGetNativeFunctionsDescriptions(AsebaVMState *vm);

// read source code to a string
// borrowed from asebatest.cpp
std::wstring read_source(const std::string& filename)
{
	if (filename == "-")
	{
		std::string line, src;
		while (std::getline(std::cin, line))
			src += line + "\n";
		return UTF8ToWString(src);
	}

	std::ifstream ifs;
	ifs.open( filename.c_str(),std::ifstream::binary);
	if (!ifs.is_open())
	{
		std::cerr << "Error opening source file " << filename << std::endl;
		exit(EXIT_FAILURE);
	}

	ifs.seekg (0, std::ios::end);
	std::streampos length = ifs.tellg();
	ifs.seekg (0, std::ios::beg);

	std::string utf8Source;
	utf8Source.resize(length);
	ifs.read(&utf8Source[0], length);
	ifs.close();

	const std::wstring s = UTF8ToWString(utf8Source);
	return s;
}

char const *readSource(char const *path)
{
	std::wstring src = read_source(std::string(path));
	std::string utf8 = WStringToUTF8(src);
	int len = utf8.length();
	char *txt = (char *)malloc(len + 1);
	memcpy(txt, utf8.c_str(), len + 1);
	return txt;
}

static TargetDescription::NativeFunction convertToNativeFunction(AsebaNativeFunctionDescription const *descr)
{
	// convert parameters
	std::vector<Aseba::TargetDescription::NativeFunctionParameter> params;
	for (int i = 0; descr->arguments[i].size != 0; i++)
	{
		auto p = TargetDescription::NativeFunctionParameter(UTF8ToWString(descr->arguments[i].name), descr->arguments[i].size);
		params.push_back(p);
	}

	// convert descr to a NativeFunction struct
	TargetDescription::NativeFunction d =
	{
		UTF8ToWString(descr->name),
		UTF8ToWString(descr->doc),
		params
	};

	return d;
}

int compileWithCompiler(AsebaVMState *vm, char const *source, Compiler &compiler)
{
	// get source code
	const std::wstring wSource = UTF8ToWString(source);
	std::wistringstream ifs(wSource);

	// compile it
	TargetDescription targetDescription;
	targetDescription.name = L"testvm";
	targetDescription.protocolVersion = ASEBA_PROTOCOL_VERSION;
	targetDescription.bytecodeSize = vm->bytecodeSize;
	targetDescription.variablesSize = vm->variablesSize;
	targetDescription.stackSize = vm->stackSize;
	const AsebaNativeFunctionDescription * const *natFunctions = AsebaGetNativeFunctionsDescriptions(vm);
	for (int i = 0; natFunctions[i]; i++)
		targetDescription.nativeFunctions.push_back(convertToNativeFunction(natFunctions[i]));
	compiler.setTargetDescription(&targetDescription);

	CommonDefinitions definitions;
	compiler.setCommonDefinitions(&definitions);

	BytecodeVector bytecode;
	unsigned int varCount;
	Error outError;
	compiler.compile(ifs, bytecode, varCount, outError, nullptr);
	if (outError.message != L"not defined")	// like in asebatest.c :-(
	{
		std::wcout << outError.toWString() << std::endl;
		return 1;
	}

	// copy bytecode to vm
	if (bytecode.size() > vm->bytecodeSize)
		return 2;
	for (int i = 0; i < bytecode.size(); i++)
		vm->bytecode[i] = bytecode[i];
	vm->bytecodeSize = bytecode.size();
	return 0;
}

int compile(AsebaVMState *vm, char const *source)
{
	Compiler compiler;
	return compileWithCompiler(vm, source, compiler);
}
