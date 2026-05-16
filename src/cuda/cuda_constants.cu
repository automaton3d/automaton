// cuda_constants.cu - Define CUDA constant memory (SINGLE DEFINITION)
#include <cuda_runtime.h>
#include <cstdio>

// Define constant memory symbols (accessible from other .cu files via extern)
__constant__ unsigned dev_EL;
__constant__ unsigned dev_W_USED;
__constant__ unsigned dev_RMAX;
__constant__ unsigned dev_CENTER;
__device__ int dev_ctrl;

// Host function to set all constants
extern "C" void setCudaConstants(unsigned EL, unsigned W_USED, unsigned RMAX) 
{
    cudaError_t err;
    unsigned CENTER = (EL - 1) / 2;
    
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
    
    printf("Setting dev_CENTER = %u\n", CENTER);
    err = cudaMemcpyToSymbol(dev_CENTER, &CENTER, sizeof(unsigned));
    if (err != cudaSuccess) {
        fprintf(stderr, "Error setting dev_CENTER: %s (code %d)\n", 
                cudaGetErrorString(err), err);
        return;
    }
    
    // Initialize dev_ctrl to 1 (active) at startup
    int initCtrl = 1;
    err = cudaMemcpyToSymbol(dev_ctrl, &initCtrl, sizeof(int));
    if (err != cudaSuccess) {
        fprintf(stderr, "Error setting dev_ctrl: %s (code %d)\n", 
                cudaGetErrorString(err), err);
        return;
    }
    
    printf("All constants set successfully\n");
}

// Reset the one‑shot control flag to 1 (call this whenever simulation restarts)
extern "C" void resetCudaCtrl()
{
    int val = 1;
    cudaError_t err = cudaMemcpyToSymbol(dev_ctrl, &val, sizeof(int));
    if (err != cudaSuccess) {
        fprintf(stderr, "Error resetting dev_ctrl: %s (code %d)\n", 
                cudaGetErrorString(err), err);
    } else {
        printf("dev_ctrl reset to 1\n");
    }
}