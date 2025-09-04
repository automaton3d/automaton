#include <cuda_runtime.h>
#include <iostream>

// CUDA kernel for vector addition
__global__ void add(int *a, int *b, int *c, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < n) c[idx] = a[idx] + b[idx];
}

int main() {
    const int N = 1024;
    int a[N], b[N], c[N];
    int *d_a, *d_b, *d_c;

    // Initialize vectors
    for (int i = 0; i < N; i++) {
        a[i] = i;
        b[i] = 2 * i;
    }

    // Allocate GPU memory
    cudaMalloc(&d_a, N * sizeof(int));
    cudaMalloc(&d_b, N * sizeof(int));
    cudaMalloc(&d_c, N * sizeof(int));

    // Copy data to GPU
    cudaMemcpy(d_a, a, N * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, b, N * sizeof(int), cudaMemcpyHostToDevice);

    // Launch kernel (N threads, 256 threads per block)
    add<<<(N + 255) / 256, 256>>>(d_a, d_b, d_c, N);

    // Copy results back to CPU
    cudaMemcpy(c, d_c, N * sizeof(int), cudaMemcpyDeviceToHost);

    // Display results
    for (int i = 0; i < 10; i++) {
        std::cout << "c[" << i << "] = " << c[i] << "\n";
    }

    // Free GPU memory
    cudaFree(d_a);
    cudaFree(d_b);
    cudaFree(d_c);

    return 0;
}
