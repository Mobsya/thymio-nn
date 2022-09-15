/*
    Copyright 2017-2022 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
    Miniature Mobile Robots group, Switzerland
    Author: Yves Piguet

    Licensed under the 3-Clause BSD License;
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at
    https://opensource.org/licenses/BSD-3-Clause
*/

// Simple shell for Aseba VM

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vm/vm.h"
#include "vm/natives.h"
#include "common/productids.h"
#include "common/consts.h"

#include "compHelper.h"
#include "disassembler.h"

#include "nn-natives.h"

// dummy implementation of required functions
// (targets/dummy/dummynode.cpp would be fine, but C is ok)
void AsebaSendBuffer(AsebaVMState *vm, const uint8_t* data, uint16_t length)
{
}

int AsebaHandleDeviceInfoMessages(AsebaVMState*, uint16_t, uint16_t*, uint16_t) {
    return 1;
}

uint16_t AsebaGetBuffer(AsebaVMState *vm, uint8_t* data, uint16_t maxLength, uint16_t* source)
{
	return 0;	// size
}

static AsebaVMDescription nodeDescription = {
	"",
	{
		{ 1, "id" },
		{ 1, "source" },
		{ 32, "args" },
		{ 1, ASEBA_PID_VAR_NAME },
		{ 1, "timer.period" },
		{ 0, NULL }
	}
};

const AsebaVMDescription* AsebaGetVMDescription(AsebaVMState *vm)
{
	return &nodeDescription;
}

void AsebaPutVmToSleep(AsebaVMState *vm)
{
}

static AsebaNativeFunctionDescription const natFunDescrDisplay =
{
	"test.display",
	"display the value",
	{
		{ -1, "value to display" },
		{ 0, NULL }
	}
};

static const AsebaNativeFunctionDescription* nativeFunctionsDescriptions[] =
{
	NN_NATIVES_DESCRIPTIONS,
	&natFunDescrDisplay,
	0
};

const AsebaNativeFunctionDescription * const * AsebaGetNativeFunctionsDescriptions(AsebaVMState *vm)
{
	return nativeFunctionsDescriptions;
}

static const AsebaLocalEventDescription localEvents[] = {
	{ "localEvent", "local event" },
	{ NULL, NULL }
};

const AsebaLocalEventDescription * AsebaGetLocalEventsDescriptions(AsebaVMState *vm)
{
	return localEvents;
}

static void test_display(AsebaVMState *vm)
{
	uint16_t addr = AsebaNativePopArg(vm);
	uint16_t size = AsebaNativePopArg(vm);
	printf("[");
	for (int i = 0; i < size; i++)
		printf("%s%d", i > 0 ? ", " : "", vm->variables[addr + i]);
	printf("]\n");
}

static AsebaNativeFunctionPointer nativeFunctions[] = {
	NN_NATIVES_FUNCTIONS,
	test_display
};

void AsebaNativeFunction(AsebaVMState *vm, uint16_t id)
{
	nativeFunctions[id](vm);
}

void AsebaWriteBytecode(AsebaVMState *vm)
{
}

void AsebaResetIntoBootloader(AsebaVMState *vm)
{
}

/**	Initialize Aseba VM
	@param[out] vm Aseba VM
	@param[in] bytecodeSize max size of bytecode
	@param[in] variablesSize max size for variables
	@param[in] stackSize max size for stack
*/
static void setupVM(AsebaVMState *vm,
	uint16_t bytecodeSize, uint16_t variablesSize, uint16_t stackSize)
{
	vm->nodeId = 0;
	vm->bytecodeSize = bytecodeSize;
	vm->bytecode = malloc(vm->bytecodeSize * sizeof(int16_t));
	vm->variablesSize = variablesSize;
	vm->variables = malloc(vm->variablesSize * sizeof(int16_t));
	vm->variablesOld = malloc(vm->variablesSize * sizeof(int16_t));
	vm->stackSize = stackSize;
	vm->stack = malloc(vm->stackSize * sizeof(int16_t));
	AsebaVMInit(vm);
}

/**	Run the init event
	@param[in,out] vm Aseba VM
*/
static void run(AsebaVMState *vm)
{
	uint16_t steps;

	AsebaVMSetupEvent(vm, ASEBA_EVENT_INIT);
	(void)AsebaVMRun(vm, 0);
}

/**	Main function
	@param[in] argc size of argv
	@param[in] argv array of arguments (argv[0] is the command name)
*/
int main(int argc, char **argv)
{
	int i;
	AsebaVMState vm;
	char const *srcPath = NULL;	// path of input source code
	char const *srcCode = NULL;	// input source code
	char const *aboPath = NULL;	// path of input abo file
	char const *bytecodePath = NULL;	// path of output bytecode
	int disass = 0;	// 1 to disassemble

	for (i = 1; i < argc; i++)
		if (strcmp(argv[i], "--dis") == 0)
			disass = 1;
		else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc)
			bytecodePath = argv[++i];
		else if (strcmp(argv[i], "--src") == 0 && i + 1 < argc)
			srcPath = argv[++i];
		else if (strcmp(argv[i], "--code") == 0 && i + 1 < argc)
			srcCode = argv[++i];
		else if (strcmp(argv[i], "--abo") == 0 && i + 1 < argc)
			aboPath = argv[++i];
		else if (strcmp(argv[i], "--info") == 0)
		{
			setupVM(&vm, 1024, 1024, 1024);
			printf("bytecodeSize: %d\n", vm.bytecodeSize);
			printf("variablesSize: %d\n", vm.variablesSize);
			printf("stackSize: %d\n", vm.stackSize);

			printf("\nNative functions:\n");
			const AsebaNativeFunctionDescription * const *natfun
				= AsebaGetNativeFunctionsDescriptions(&vm);
			for (int i = 0; natfun[i]; i++)
			{
				printf("%s\n", natfun[i]->name);
				printf("%s\n", natfun[i]->doc);
				for (int j = 0; natfun[i]->arguments[j].size; j++)
					printf("  %s [%d]\n",
						natfun[i]->arguments[j].name, natfun[i]->arguments[j].size);
			}

			exit(0);
		}
		else
		{
			printf(
				"Usage: %s [OPTION]...\n"
				"Run local Aseba VM.\n"
				"  --abo         input .abo file\n"
				"  --code 'code' compile, load and execute Aseba source code\n"
				"  --dis         show disassembly code before executing it\n"
				"  --help        display this message and exit\n"
				"  --info        display information about node\n"
				"  --out file    output file bytecode is written to\n"
				"  --src file    compile, load and execute Aseba source code\n"
				, argv[0]);
			exit(strcmp(argv[i], "--help") == 0 ? 0 : 1);
		}

	setupVM(&vm, 1024, 1024, 1024);
	if (srcPath || srcCode)
	{
		char const *src = srcCode ? srcCode : readSource(srcPath);

		if (compile(&vm, src) != 0)
		{
			fprintf(stderr, "Compilation error.\n");
			exit(1);
		}
		if (disass)
			// disassemble(vm.bytecode, vm.bytecodeSize);
			compileAndDisassemble(&vm, src);
		if (bytecodePath)
		{
			FILE *fp = fopen(bytecodePath, "wb");
			if (!fp)
			{
				fprintf(stderr, "Cannot open file \"%s\"\n", bytecodePath);
				exit(1);
			}
			fwrite(vm.bytecode, 1, vm.bytecodeSize, fp);
			fclose(fp);
		}
		run(&vm);
	}
	else if (aboPath)
	{
		FILE *fp;
		char header[20];
		int len;

		fp = fopen(aboPath, "rb");
		if (!fp)
		{
			fprintf(stderr, "Cannot open file \"%s\"\n", aboPath);
			exit(1);
		}
		if (fread(header, 1, 20, fp) != 20)
		{
			fprintf(stderr, "Cannot read abo header\n");
			exit(1);
		}
		if (header[0] != 'A' || header[1] != 'B' || header[2] != 'O' || header[3] != '\0')
		{
			fprintf(stderr, "Bad abo header\n");
			exit(1);
		}
		if (header[4] != 0 || header[5] != '\0')
		{
			fprintf(stderr, "Bad abo version\n");
			exit(1);
		}
		len = ((unsigned char)header[0x12] | (int)(unsigned char)header[0x13] << 8);
		printf("Bytecode length: %d\n", len);

		if (disass)
		{
			uint16_t *bytecode = malloc(2 * len);
			if (!bytecode)
			{
				fprintf(stderr, "Cannot allocate %d bytes for bytecode\n", 2 * len);
				exit(1);
			}
			fseek(fp, 0x14, SEEK_SET);
			fread(bytecode, 2, len, fp);
			disassemble(bytecode, len);
		}
		fclose(fp);
	}

	return 0;
}
