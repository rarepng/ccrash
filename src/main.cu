#include <iostream>
#include <cuda_runtime.h>

__global__ void helloFromGPU() { printf("block %d thread %d reporting in 🫡\n", blockIdx.x, threadIdx.x); }

int main() {
	cudaDeviceSetLimit(cudaLimitPrintfFifoSize, 4096);

	helloFromGPU<<<2, 32>>>();

	cudaError_t err = cudaGetLastError();
	if (err != cudaSuccess) {
		std::cerr << "error: " << cudaGetErrorString(err) << std::endl;
		return -1;
	}

	cudaDeviceSynchronize();
	return 0;
}
