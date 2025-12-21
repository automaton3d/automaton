/*
 * cuda.h
 *
 *  Created on: 16 de dez. de 2025
 *      Author: Alexandre
 */

#ifdef HBNMMM

#ifndef INCLUDE_MODEL_CUDA_H_
#define INCLUDE_MODEL_CUDA_H_

#include <vector>
#include "simulation.h"

#define cudaSuccess      0

typedef int cudaError_t;

cudaError_t cudaGetDeviceCount(int* count);
void cudaSetDevice(int device);
void initGPU(int a, int b);
void runSimulationSteps(int steps);
void cudaDeviceSynchronize();
void getLayerData(unsigned selectedW, std::vector<automaton::Cell>& host_layer_data);
const char* cudaGetErrorString(cudaError_t err);

#endif /* INCLUDE_MODEL_CUDA_H_ */

#endif