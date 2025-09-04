# Compiler and flags
CC := g++
NVCC := "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.1/bin/nvcc.exe"
TARGET := cuda_program
OBJ := main.o kernel.o

$(TARGET): $(OBJ)
	$(NVCC) -o $(TARGET) $(OBJ)

%.o: %.cu
	$(NVCC) -c $< -o $@

clean:
	del /Q $(TARGET).exe *.o
