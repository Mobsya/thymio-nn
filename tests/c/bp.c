/*
	Copyright 2022 ECOLE POLYTECHNIQUE FEDERALE DE LAUSANNE,
	Miniature Mobile Robots group, Switzerland
	Author: Yves Piguet

	Licensed under the 3-Clause BSD License;
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at
	https://opensource.org/licenses/BSD-3-Clause
*/

// test of nn implementation: learning with backpropagation

#include "nn/nn.h"
#include "nn/nn-alloc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define maxLineLength 1024

static int countFileLines(char const *path) {
	FILE *fp = fopen(path, "r");
	if (fp == NULL) {
		fprintf(stderr, "Cannot open file %s\n", path);
		exit(1);
	}

	int count = 0;
	for (count = 0; !feof(fp); ) {
		char line[maxLineLength];
		if (fgets(line, maxLineLength, fp)) {
			count++;
		}
	}
	fclose(fp);
	return count;
}

static void loadCSV(char const *path, NNObservations *obs) {
	double x;
	int i;
	NNFloat *input, *output;

	FILE *fp = fopen(path, "r");
	if (fp == NULL) {
		fprintf(stderr, "Cannot open file %s\n", path);
		exit(1);
	}

	for (obs->count = i = 0;;) {
		int r = fscanf(fp, "%lf,", &x);
		if (r == 1) {
			NNObservationGetPtr(obs, obs->count, &input, &output);
			if (i < obs->inputCount) {
				input[i++] = (NNFloat)x;
			} else {
				output[i++ - obs->inputCount] = (NNFloat)x;
				if (i >= obs->inputCount + obs->outputCount) {
					i = 0;
					obs->count++;
				}
			}
		} else {
			break;	// eof
		}
	}

	fclose(fp);
}

int main(int argc, char **argv) {
	NN nn = { 0, 0, 0, 0, 0 };   // empty
	NNBackProp bp = { 0, 0, 0, 0, 0, 0 };	// empty
	NNObservations obs = { 0, 0, 0, 0, 0 };	// empty
	int layerCount;
	int inputCount;
	int maxIter = 1;
	NNFloat eta = 0.02;
	void *backpropTempMem = 0;
	char const *trainingDatasetPath = NULL;
	char const *validationDatasetPath = NULL;
	NNFloat errormax = -1;	// default: no check
	int obsCount = 0;
	int verbose = 0;
	int quiet = 0;

	// count layers and decode --input
	layerCount = 0;
	inputCount = 0;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
			inputCount = strtol(argv[++i], NULL, 0);
		} else if (strcmp(argv[i], "--layer") == 0) {
			layerCount++;
		}
	}

	NNReset(&nn, layerCount);

	int nextLayerInputCount = inputCount;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--errormax") == 0 && i + 1 < argc) {
			errormax = strtod(argv[++i], NULL);
		} else if (strcmp(argv[i], "--eta") == 0 && i + 1 < argc) {
			eta = strtod(argv[++i], NULL);
		} else if (strcmp(argv[i], "--input") == 0 && i + 1 < argc) {
			i++;	// already parsed
		} else if (strcmp(argv[i], "--iter") == 0 && i + 1 < argc) {
			maxIter = strtol(argv[++i], NULL, 0);
		} else if (strcmp(argv[i], "--layer") == 0 && i + 2 < argc) {
			int outputCount = strtol(argv[++i], NULL, 0);
			i++;
			NNActivation act = NNActivationIdentity;
			if (strcmp(argv[i], "tanh") == 0) {
				act = NNActivationTanh;
			} else if (strcmp(argv[i], "sigmoid") == 0) {
				act = NNActivationSigmoid;
			} else if (strcmp(argv[i], "identity") != 0 && strcmp(argv[i], "none") != 0) {
				fprintf(stderr, "Unknown activation function %s\n", argv[i]);
				exit(1);
			}
			NNAddLayer(&nn, nextLayerInputCount, outputCount, act);
			nextLayerInputCount = outputCount;
		} else if (strcmp(argv[i], "--quiet") == 0) {
			quiet = 1;
		} else if (strcmp(argv[i], "--training") == 0 && i + 1 < argc) {
			trainingDatasetPath = argv[++i];
		} else if (strcmp(argv[i], "--validation") == 0 && i + 1 < argc) {
			validationDatasetPath = argv[++i];
		} else if (strcmp(argv[i], "--verbose") == 0) {
			verbose = 1;
		} else {
			printf("Usage: %s [options]\n"
				"\n"
				"Options:\n"
				"  --errormax x       maximum error accepted for validation\n"
				"                     (default: no maximum)\n"
				"  --eta x            eta learning rate\n"
				"  --help             display this message and exit\n"
				"  --input n          number of inputs\n"
				"  --iter n           number of iterations\n"
				"  --layer n a        layer description with number of outputs n\n"
				"                     and activation a (\"identity\", \"tanh\" or \"sigmoid\")\n"
				"  --quiet            suppress output\n"
				"  --training path    dataset used for training (csv file where each row contains\n"
				"                     the inputs and outputs of one observation)\n"
				"  --validation path  dataset used for validation (csv file where each row contains\n"
				"                     the inputs and outputs of one observation)\n"
				"  --verbose          display diagnostic information\n"
				, argv[0]);
			exit(0);
		}
	}

	if (verbose) {
		printf("Number of inputs: %d\n", nn.inputCount);
		printf("Number of outputs: %d\n", nn.outputCount);
		printf("Number of layers: %d\n", nn.layerCount);
		printf("Layers:\n");
		for (int k = 0; k < nn.layerCount; k++) {
			printf("  in=%d out=%d %s\n",
				nn.layer[k].inputCount, nn.layer[k].outputCount,
				nn.layer[k].activation == NNActivationIdentity ? "identity" : nn.layer[k].activation == NNActivationTanh ? "tanh" : nn.layer[k].activation == NNActivationSigmoid ? "sigmoid" : "???");
		}
		printf("\n");
	}

	NNInitWeights(&nn);

	NNBackPropAllocStorage(&nn, &backpropTempMem);
	NNBackPropInit(&nn, &bp, backpropTempMem);

	if (trainingDatasetPath) {
		obsCount = countFileLines(trainingDatasetPath);
		if (obsCount > 0) {
			NNObservationsInit(&obs, nn.inputCount, nn.outputCount, obsCount);
			loadCSV(trainingDatasetPath, &obs);
			if (verbose) {
				printf("Size of dataset used for training: %d\n", obs.count);
				printf("Number of steps for training: %d\n", maxIter);
				printf("Learning rate eta: %g\n", eta);
			}
			NNFloat costInitial = 0;
			NNFloat costFinal = 0;
			if (obs.count > 0) {
				for (int i = 0; i < maxIter; i++) {
					NNFloat *input, *output;
					NNObservationGetPtr(&obs, i % obs.count, &input, &output);

					NNFloat *nnInput = NNGetInputPtr(&nn);
					NNFloat *nnOutput = NNGetOutputPtr(&nn);
					for (int j = 0; j < nn.inputCount; j++) {
						nnInput[j] = input[j];
					}
					if (i < obs.count) {
						NNEval(&nn, NULL);
						costInitial += NNBackPropCost(&nn, output);
					} else if (i >= (maxIter / obs.count - 1) * obs.count && i < (maxIter / obs.count) * obs.count) {
						NNEval(&nn, NULL);
						costFinal += NNBackPropCost(&nn, output);
					}
					for (int j = 0; j < nn.outputCount; j++) {
						nnOutput[j] = output[j];
					}

					NNBackPropResetGradients(&nn, &bp);
					NNBackPropAddGradients(&nn, &bp);
					NNBackPropApply(&nn, &bp, eta);
				}
				if (verbose) {
					printf("Initial cost: %g\n", costInitial);
					printf("Final cost: %g\n", costFinal);
				}
			}
		}
	}

	if (validationDatasetPath) {
		obsCount = countFileLines(validationDatasetPath);
		if (obsCount > 0) {
			NNObservationsInit(&obs, nn.inputCount, nn.outputCount, obsCount);
			loadCSV(validationDatasetPath, &obs);
			if (verbose) {
				printf("\nSize of dataset used for validation: %d\n", obs.count);
			}
			if (obs.count > 0) {
				for (int i = 0; i < obs.count; i++) {
					NNFloat *input, *output;
					NNObservationGetPtr(&obs, i, &input, &output);

					NNFloat *nnInput = NNGetInputPtr(&nn);
					NNFloat *nnOutput = NNGetOutputPtr(&nn);
					for (int j = 0; j < nn.inputCount; j++) {
						nnInput[j] = input[j];
					}
					NNEval(&nn, NULL);

					if (!quiet) {
						printf("Expected: ");
						for (int j = 0; j < nn.outputCount; j++) {
							printf("%8.2f", output[j]);
						}
						printf("\nNN:       ");
						for (int j = 0; j < nn.outputCount; j++) {
							printf("%8.2f", nnOutput[j]);
						}
						printf("\n");
					}

					if (errormax >= 0) {
						int failed = 0;
						for (int j = 0; !failed && j < nn.outputCount; j++) {
							failed = fabs(output[j] - nnOutput[j]) > errormax;
						}
						if (failed) {
							if (!quiet) {
								printf("Validation failure\n");
							}
							exit(1);
						}
					}
				}
			}
		}
	}

	return 0;
}
