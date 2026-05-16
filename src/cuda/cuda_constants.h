#pragma once
#include <cuda_runtime.h>

extern __constant__ unsigned dev_EL;
extern __constant__ unsigned dev_W_USED;
extern __constant__ unsigned dev_RMAX;
extern __constant__ unsigned dev_CENTER;
extern __device__ int dev_ctrl;   // note: __device__

#ifdef __cplusplus
extern "C" {
#endif
void setCudaConstants(unsigned EL, unsigned W_USED, unsigned RMAX);
void resetCudaCtrl();
#ifdef __cplusplus
}
#endif