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


#include <stdlib.h>
#include <stdio.h>

#include "common/consts.h"
#if defined(USE_COMPILER)
#	include "compiler/compiler.h"
#endif
#include "common/utils/utils.h"

#include "disassembler.h"
#include "compHelper.h"

using namespace Aseba;

#if defined(USE_COMPILER)
static bool findVar(Aseba::Compiler *compiler, int varIx, std::string &str)
{
	if (!compiler)
		return false;

	auto map = compiler->getVariablesMap();
	for (auto it = map->begin(); it != map->end(); it++)
	{
		auto name = it->first;
		auto offset = it->second.first;
		auto size = it->second.second;
		if (varIx >= offset && varIx < offset + size)
		{
			// found
			std::ostringstream strs;
			strs << WStringToUTF8(name);
			if (varIx > offset)
				strs << "[" << (varIx - offset) << "]";
			str = strs.str();
			return true;
		}
	}
	return false;
}

static bool findFunction(Aseba::Compiler *compiler, int funIx, std::string &str)
{
	if (!compiler)
		return false;

	const TargetDescription *descr = compiler->getTargetDescription();
	if (false && funIx < descr->nativeFunctions.size())
	{
		str = WStringToUTF8(descr->nativeFunctions.begin()[funIx].name);
		return true;
	}
	return false;
}
#else
static bool findVar(void *compiler, int varIx, std::string &str)
{
	return false;
}

static bool findFunction(void *compiler, int funIx, std::string &str)
{
	return false;
}
#endif

void disassembleWithCompiler(uint16_t const *bytecode, uint16_t bytecodeSize,
#if defined(USE_COMPILER)
	Aseba::Compiler *compiler
#else
	void *compiler
#endif
)
{
	int i, j;
	int eventId;	// -1=none
	int eventVectorSize = bytecode[0];
	std::string str;
	static char const * const condName[] =
	{
		"shift left",
		"shift right",
		"add",
		"sub",
		"mult",
		"div",
		"mod",
		"bitor",
		"bitxor",
		"bitand",
		"eq",
		"ne",
		"gt",
		"ge",
		"lt",
		"le",
		"or",
		"and"
	};

	// event dispatch table
	printf("%5d  dc %d\n", 0, eventVectorSize);
	for (i = 1; i < eventVectorSize; i += 2)
		printf("%5d  dc %d, %d\n", i, bytecode[i], bytecode[i + 1]);

	for (i = eventVectorSize; i < bytecodeSize; i++)
	{
		// find event, if address match
		eventId = -1;
		for (j = 0; j < eventVectorSize; j += 2)
			if (i == bytecode[1 + j + 1])
			{
				eventId = bytecode[1 + j];
				break;
			}

		// display event and address
		if (eventId == ASEBA_EVENT_INIT)
			printf("event_init:\n");
		else if (eventId >= 0)
			printf("event_%d:\n", eventId);
		printf("%5d  ", i);

		// decode opcode
		uint16_t op = bytecode[i];
		switch (op >> 12)
		{
			case ASEBA_BYTECODE_STOP:
				printf("%.4x       stop\n", op);
				break;
			case ASEBA_BYTECODE_SMALL_IMMEDIATE:
				printf("%.4x       push.s %d\n", op, (int)(op & 0xfff));
				break;
			case ASEBA_BYTECODE_LARGE_IMMEDIATE:
				printf("%.4x %.4x  push %d\n", op, bytecode[i + 1], (int)bytecode[++i]);
				break;
			case ASEBA_BYTECODE_LOAD:
				if (findVar(compiler, op & 0xfff, str))
					printf("%.4x       load %s\n", op, str.c_str());
				else
					printf("%.4x       load %d\n", op, (int)(op & 0xfff));
				break;
			case ASEBA_BYTECODE_STORE:
				if (findVar(compiler, op & 0xfff, str))
					printf("%.4x       store %s\n", op, str.c_str());
				else
					printf("%.4x       store %d\n", op, (int)(op & 0xfff));
				break;
			case ASEBA_BYTECODE_LOAD_INDIRECT:
				printf("%.4x %.4x  load %d+ind size=%d\n", op, bytecode[i + 1], (int)(op & 0xfff), (int)bytecode[++i]);
				break;
			case ASEBA_BYTECODE_STORE_INDIRECT:
				printf("%.4x %.4x  store %d+ind size=%d\n", op, bytecode[i + 1], (int)(op & 0xfff), (int)bytecode[++i]);
				break;
			case ASEBA_BYTECODE_JUMP:
				printf("%.4x       jump %d\n", op, i + (int16_t)(op & 0xfff | (op & 800 ? 0xf000 : 0)));
				break;
			case ASEBA_BYTECODE_CONDITIONAL_BRANCH:
				// self-modifying code :-(
				// when: last state stored in bit ASEBA_IF_WAS_TRUE_BIT
				if ((op & 0xfff) < sizeof(condName) / sizeof(condName[0]))
					printf("%.4x %.4x  jump %s not %s %d\n",
						op, bytecode[i + 1],
						op & (1 << ASEBA_IF_IS_WHEN_BIT) ? "when" : "if",
						condName[op & 0xfff], i + (int16_t)bytecode[i + 1]);
				else
					printf("%.4x %.4x  jump %s not cond ?? %d\n",
						op, bytecode[i + 1],
						op & (1 << ASEBA_IF_IS_WHEN_BIT) ? "when" : "if",
						i + (int16_t)bytecode[i + 1]);
				i++;
				break;
			case ASEBA_BYTECODE_EMIT:
				printf("%.4x %.4x  emit id=%d data=%d count=%d\n",
					op, bytecode[i + 1],
					(int)(op & 0xfff), (int)bytecode[i + 1], (int)bytecode[i + 2]);
				i += 2;
				break;
			case ASEBA_BYTECODE_NATIVE_CALL:
				if (findFunction(compiler, op & 0xfff, str))
					printf("%.4x       callnat %s\n", op, str.c_str());
				else
					printf("%.4x       callnat %d\n", op, (int)(op & 0xfff));
				break;
			case ASEBA_BYTECODE_SUB_CALL:
				printf("%.4x       call %d\n", op, (int)(op & 0xfff));
				break;
			case ASEBA_BYTECODE_SUB_RET:
				printf("%.4x       ret\n", op);
				break;
			default:
				printf("%.4x       ", op);
				switch (op)
				{
					case (ASEBA_BYTECODE_UNARY_ARITHMETIC << 12) | ASEBA_UNARY_OP_SUB:
						printf("negate\n");
						break;
					case (ASEBA_BYTECODE_UNARY_ARITHMETIC << 12) | ASEBA_UNARY_OP_ABS:
						printf("abs\n");
						break;
					case (ASEBA_BYTECODE_UNARY_ARITHMETIC << 12) | ASEBA_UNARY_OP_BIT_NOT:
						printf("bitnot\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_SHIFT_LEFT:
						printf("shift left\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_SHIFT_RIGHT:
						printf("shift right\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_ADD:
						printf("add\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_SUB:
						printf("sub\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_MULT:
						printf("mult\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_DIV:
						printf("div\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_MOD:
						printf("mod\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_BIT_OR:
						printf("bitor\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_BIT_XOR:
						printf("bitxor\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_BIT_AND:
						printf("bitand\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_EQUAL:
						printf("eq\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_NOT_EQUAL:
						printf("ne\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_BIGGER_THAN:
						printf("gt\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_BIGGER_EQUAL_THAN:
						printf("ge\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_SMALLER_THAN:
						printf("lt\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_SMALLER_EQUAL_THAN:
						printf("le\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_OR:
						printf("or\n");
						break;
					case (ASEBA_BYTECODE_BINARY_ARITHMETIC << 12) | ASEBA_OP_AND:
						printf("and\n");
						break;
					default:
						printf("?? (%4x)\n", (int)op);
						break;
				}
				break;
		}
	}
}

#if defined(USE_COMPILER)
void compileAndDisassemble(AsebaVMState *vm, char const *source)
{
	Aseba::Compiler compiler;
	compileWithCompiler(vm, source, compiler);
	disassembleWithCompiler(vm->bytecode, vm->bytecodeSize, &compiler);
}
#endif

void disassemble(uint16_t const *bytecode, uint16_t bytecodeSize)
{
	disassembleWithCompiler(bytecode, bytecodeSize, nullptr);
}
