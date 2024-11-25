/*
 * utils.cpp
 *
 * Ancillary code.
 */

#include "simulation.h"
#include "../mygl.h"
#include <cmath>
#include <vector>

namespace automaton
{
	unsigned last_n = 0;
	extern std::vector<Cell> lattice_current;
	COLORREF *voxels;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(0, 99);

	int w = 0;  // Global variable for the w shell to process

	/**
	 * This is a bridge between the model and the graphical framework.
	 */
	void updateBuffer()
	{
	    // Ensure that the cell's k value is not 1 for the selected layer
	   // if (lattice_current[0].k == 1)  // Assuming 0 is a valid index to check, adjust as needed
	     //   return;

	    // Check if the framework is active and if there are any checkboxes
	    if (!framework::active || framework::checkboxes.empty())
	        return;

	    // Find the selected layer
	    int w = -1; // Initialize to -1 to indicate no layer selected
	    for (int i = 0; i < LAYERS; i++)
	    {
	        if (framework::layers[i].isSelected())
	        {
	            w = i; // Set the selected layer
	            break; // Exit loop once the layer is found
	        }
	    }

	    // If no layer is selected, exit the function
	    if (w == -1)
	        return;
	    // Pointer to the first cell in the selected layer
	    Cell* cell = &lattice_current[w * SIDE3];
	    // Iterate over the 3D grid to update the voxel data
	    for (int x = 0; x < SIDE; x++)
	    {
	        for (int y = 0; y < SIDE; y++)
	        {
	            for (int z = 0; z < SIDE; z++)
	            {
	                // Calculate the 1D index for the current cell in the 3D grid
	                int index3D = (x * SIDE2) + (y * SIDE) + z;

	                // Check if 'wv' is a valid property and update the voxel color
	                if (cell[index3D].wv)  // Corrected to access the correct cell
	                {
	                    voxels[index3D] = RGB(255, 255, 255);  // White voxel
	                }
	                else
	                {
	                    voxels[index3D] = RGB(0, 0, 0);        // Black voxel
	                }
	            }
	        }
	    }
	}

	void displayLattice()
    {
		system("cls");
		printf("DIFFUSE=%d\tSHIFTS=%d\tRMAX=%d\tUNIQUE=%d\tstep=%d\n", XYZ_DIFFUSION, RELOC, RMAX, LIGHT, lattice_current[0].k);

        // Cabeçalho da grade
        for (int i = 0; i < SIDE; i++)
            printf("L:%d\t\t\t", i);
        printf("\n");

        printf("w: %d\n", w); // Imprime o valor da dimensão w
        // Iteração sobre as linhas y
        for (int y = 0; y < SIDE; y++)
        {
            // Iteração sobre as camadas z
            for (int z = 0; z < SIDE; z++)
            {
                // Iteração sobre as colunas x
                for (int x = 0; x < SIDE; x++)
                {
                    // Cálculo direto do índice
                    int index3D = x * SIDE * SIDE + y * SIDE + z;
//#define PATTERN
#ifdef PATTERN
                    printf("[%3u,%d]", (lattice_current + index3D)->d, (lattice_current + index3D)->m);
#else
                    if (lattice_current[index3D].wv)
                        printf("X ");
                    else
                        printf(". ");
#endif
                }
                std::cout << "   ";  // Adiciona espaço entre as camadas
            }
            std::cout << std::endl;  // Move para a próxima linha após todas as camadas
        }
        puts(" "); // Espaço entre as iterações de w
        Sleep(50);  // Ajuste o tempo de pausa se necessário
    }

	Cell* get_neighbor(int index, int dir)
	{
	    // Extract the 3D coordinates (x, y, z) and shell (w) from the flattened index
	    int x = (index / SIDE2) % SIDE;
	    int y = (index / SIDE) % SIDE;
	    int z = index % SIDE;
	    int shell = index / SIDE3;  // This should correspond to the depth layer (W_DIM)

	    // Calculate the new index based on the direction
	    switch (dir)
	    {
	        case NORTH:
	            x = (x - 1 + SIDE) % SIDE;  // Wrap around on the x-axis
	            break;

	        case EAST:
	            y = (y + 1) % SIDE;  // Wrap around on the y-axis
	            break;

	        case SOUTH:
	            x = (x + 1) % SIDE;  // Wrap around on the x-axis
	            break;

	        case WEST:
	            y = (y - 1 + SIDE) % SIDE;  // Wrap around on the y-axis
	            break;

	        case UP:
	            z = (z + 1) % SIDE;  // Wrap around on the z-axis
	            break;

	        case DOWN:
	            z = (z - 1 + SIDE) % SIDE;  // Wrap around on the z-axis
	            break;

	        default:
	            return nullptr;  // Return nullptr if direction is invalid
	    }

	    // Calculate the new index based on the modified coordinates and shell
	    int new_index = (x * SIDE2 + y * SIDE + z) + shell * SIDE3;

	    // Return the pointer to the neighboring cell
	    return &lattice_current[new_index];
	}

	int get_neighbor_index(int index, int dir)
	{
	    // Extract x, y, z, and shell from the given index
	    int x = (index / SIDE2) % SIDE;
	    int y = (index / SIDE) % SIDE;
	    int z = index % SIDE;
	    int shell = index / SIDE3;

	    // Calculate the neighbor index based on the direction
	    switch (dir)
	    {
	        case NORTH:
	            return (((x - 1 + SIDE) % SIDE) * SIDE2 + y * SIDE + z) + shell * SIDE3;
	        case EAST:
	            return (x * SIDE2 + ((y + 1) % SIDE) * SIDE + z) + shell * SIDE3;
	        case SOUTH:
	            return (((x + 1) % SIDE) * SIDE2 + y * SIDE + z) + shell * SIDE3;
	        case WEST:
	            return (x * SIDE2 + ((y - 1 + SIDE) % SIDE) * SIDE + z) + shell * SIDE3;
	        case UP:
	            return (x * SIDE2 + y * SIDE + ((z + 1) % SIDE)) + shell * SIDE3;
	        case DOWN:
	            return (x * SIDE2 + y * SIDE + ((z - 1 + SIDE) % SIDE)) + shell * SIDE3;
	        default:
	            return -1; // Invalid direction
	    }
	}

	bool isColorNeutral(unsigned char c1, unsigned char c2)
	{
	    unsigned res = (c1 ^ c2) & 7;
	    return ((res == 0 || res == 7) && c1 && c2 && c1 != 7 && c2 != 7);
	}

	/*
	 * Prints the CA state for test.
	 */
	void printLattice()
	{
	    for (unsigned w = 0; w < W_DIM; w++)
	    {
	        // Calculate the center index based on the 1D flattened index of (x, y, z, w)
	        int center = (CENTER * SIDE2 + CENTER * SIDE + CENTER) + w * SIDE3;
	        // Access the cell using the calculated index
	        Cell* ptr = &lattice_current[center];  // Pointer to the cell at the center position
	        // Print the 'pole' value of the current cell
	        printf("%d  ", ptr->pole);  // Assuming 'pole' is a member of Cell
	    }
	}

	void cross_product(double result[3], const double a[3], const double b[3])
	{
	    result[0] = a[1] * b[2] - a[2] * b[1];
	    result[1] = a[2] * b[0] - a[0] * b[2];
	    result[2] = a[0] * b[1] - a[1] * b[0];
	}

	void normalize(double vec[3])
	{
	    double magnitude = sqrt(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);

	    // Avoid division by zero
	    if (magnitude > 0.0)
	    {
	        vec[0] /= magnitude;
	        vec[1] /= magnitude;
	        vec[2] /= magnitude;
	    }
	}

}
