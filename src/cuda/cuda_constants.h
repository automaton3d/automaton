// cuda_constants.h - MINIMAL VERSION to break circular includes
#ifndef CUDA_CONSTANTS_H_GUARD
#define CUDA_CONSTANTS_H_GUARD

// Do NOT include any other headers here to avoid circular dependencies

// Forward declarations only
#ifdef __CUDACC__
// These are declared extern and defined in cuda_constants.cu
extern __constant__ unsigned dev_EL;
extern __constant__ unsigned dev_W_USED;
extern __constant__ unsigned dev_RMAX;
#endif

// C-linkage function declaration
#ifdef __cplusplus
extern "C" {
#endif

void setCudaConstants(unsigned EL, unsigned W_USED, unsigned RMAX);

#ifdef __cplusplus
}
#endif

#endif // CUDA_CONSTANTS_H_GUARD