// cuda_constants.cu - Define CUDA constant memory (SINGLE DEFINITION)
#include <cuda_runtime.h>
#include <cstdio>

// DEFINE the actual constant memory HERE ONLY (no extern, no include of cuda_constants.h)
__constant__ unsigned dev_EL;
__constant__ unsigned dev_W_USED;
__constant__ unsigned dev_RMAX;

// Implement the host function to set constants
extern "C" void setCudaConstants(unsigned EL, unsigned W_USED, unsigned RMAX) 
{
    cudaError_t err;
    
    printf("Setting dev_EL = %u\n", EL);
    err = cudaMemcpyToSymbol(dev_EL, &EL, sizeof(unsigned));
    if (err != cudaSuccess) {
        fprintf(stderr, "Error setting dev_EL: %s (code %d)\n", 
                cudaGetErrorString(err), err);
        return;
    }
    
    printf("Setting dev_W_USED = %u\n", W_USED);
    err = cudaMemcpyToSymbol(dev_W_USED, &W_USED, sizeof(unsigned));
    if (err != cudaSuccess) {
        fprintf(stderr, "Error setting dev_W_USED: %s (code %d)\n", 
                cudaGetErrorString(err), err);
        return;
    }
    
    printf("Setting dev_RMAX = %u\n", RMAX);
    err = cudaMemcpyToSymbol(dev_RMAX, &RMAX, sizeof(unsigned));
    if (err != cudaSuccess) {
        fprintf(stderr, "Error setting dev_RMAX: %s (code %d)\n", 
                cudaGetErrorString(err), err);
        return;
    }
    
    printf("All constants set successfully\n");
}