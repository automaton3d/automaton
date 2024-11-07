/*
 * initSim.cpp
 *
 * Gather all initialization routines.
 */

#include "simulation.h"
#include <cstring>

namespace automaton
{
	using namespace std;

	extern Cell *lattice_main, *lattice_draft, *lattice_mirror;
	int (*points)[3];
	int indx = 0;

	/*
	 * Initializes the spin vectors in two steps.
	 */
	void initSpin(Cell *lattice)
	{
	  Cell *ptr = lattice;
	  int w = 0;
	  // Fisrt step: isotropic distribution
	  for (int x = 0; x < SIDE; x++)
	  {
	    for (int y = 0; y < SIDE; y++)
	    {
	      for (int z = 0; z < SIDE; z++)
	      {
	        int dx = x - SIDE/2;
	        int dy = y - SIDE/2;
	        int dz = z - SIDE/2;
	        double d = sqrt(dx*dx + dy*dy + dz*dz);
	        if (d >= SIDE/2 && d < SIDE/2+1)
	        {
	          ptr->p[0] = x;
	          ptr->p[1] = y;
	          ptr->p[2] = z;
              ptr++;
              w++;
	        }
          }
	    }
	  }
	  // Second step: complete cells, introduce a 'defect'
      for (int x = SIDE - 1; w < 13*SIDE*SIDE/4; x--)
	  {
	    for (int y = 0; y < SIDE; y++)
	    {
	      for (int z = 0; z < SIDE; z++)
	      {
	        int dx = x - SIDE/2;
	        int dy = y - SIDE/2;
	        int dz = z - SIDE/2;
	        double d = sqrt(dx*dx + dy*dy + dz*dz);
	        if (d >= SIDE/2 && d < SIDE/2+1)
	        {
	          ptr->p[0] = x;
	          ptr->p[1] = y;
	          ptr->p[2] = z;
              ptr++;
              w++;
	        }
	      }
	    }
      }
	}

	// Function to compute the cross product of two vectors
	void cross_product(int result[3], int a[3], int b[3])
	{
	    result[0] = a[1] * b[2] - a[2] * b[1];
	    result[1] = a[2] * b[0] - a[0] * b[2];
	    result[2] = a[0] * b[1] - a[1] * b[0];
	}

	// Function to normalize a vector
	void normalize(int v[3])
	{
	    double magnitude = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	    if (magnitude == 0)
	        return; // Prevent division by zero

	    v[0] = static_cast<int>(v[0] / magnitude);
	    v[1] = static_cast<int>(v[1] / magnitude);
	    v[2] = static_cast<int>(v[2] / magnitude);
	}

	// Generate the orthogonal momentum set
	void generate_momentum_set(Cell *lattice)
	{
	    Cell *ptr = lattice;

	    for (int i = 0; i < DEPTH; i++, ptr++)
	    {
	        int *s = ptr->s; // Directly use the pointer from Cell

	        // Choose a default vector `v` to calculate perpendicular
	        int v[3] = {1, 0, 0};

	        // Adjust vector `v` if `s` is aligned along z-axis
	        if (s[0] == 0 && s[1] == 0)
	        {
	            v[0] = 0;
	            v[1] = 1; // Choose x-y plane vector
	        }

	        // Generate the first perpendicular vector
	        int p1[3];
	        cross_product(p1, s, v);
	        normalize(p1);

	        // Generate the second perpendicular vector and assign directly to `ptr->p`
	        cross_product(ptr->p, s, p1);
	        normalize(ptr->p);
	    }
	}

	/*
	 * Partial initialization of singularity.
	 * (less momentum and spin)
	 */
	void partialInit(Cell *grid)
	{
	    Cell *ptr = grid;
	    int i = 0;
	    for (int z = 0; z < SIDE; z++)
	    {
	        for (int y = 0; y < SIDE; y++)
	        {
	            for (int x = 0; x < SIDE; x++)
	            {
	                char w0 = i % 2;
	                char w1 = (i >> 1) % 2;
	                char q = w0 ^ w1;

	                ptr->charge = (i % 8) | (w0 << 3) | (1 << 4) | (q << 5);
	                ptr->aff = i + 1;     // dodges zero

	                // Use memset to initialize struct members o and po
	                memset(ptr->o, 0, sizeof(ptr->o));
	                i++;
	                ptr++;
	            }
	            ptr += SIDE2 - SIDE;
	        }
	        ptr += SIDE4 - SIDE3;
	    }
	}

    /**
     * Function to initialize the lattice.
     */
    void initLattice()
    {
        Cell *main_ptr = lattice_main;
        Cell *draft_ptr = lattice_draft;

        // Loop over the linear index in a 4D grid
        for (int index = 0; index < SIDE4; index++)
        {
            int x = (index / SIDE2) % SIDE;    // Calculate x coordinate
            int y = (index / SIDE) % SIDE;     // Calculate y coordinate
            int z = index % SIDE;              // Calculate z coordinate

            // Calculate distances relative to the center
            double dx = (double)(x - SIDE / 2);
            double dy = (double)(y - SIDE / 2);
            double dz = (double)(z - SIDE / 2);
            double d = sqrt(dx * dx + dy * dy + dz * dz);

            // Initialize cell properties
            main_ptr->n = 0;
            main_ptr->m = 0;
            main_ptr->ctrl = false;  // this guarantees segregation (6.6.7)
            main_ptr->wv = false;
            main_ptr->d = (unsigned)(LIGHT * d);
            main_ptr->reloc = false;

            // Assign cell coordinates
            main_ptr->x = x;
            main_ptr->y = y;
            main_ptr->z = z;

            main_ptr->cx = 0;
            main_ptr->cy = 0;
            main_ptr->cz = 0;

            // Define frequency and assertion
            main_ptr->freq = 2;
            assert(main_ptr->freq < FMAX);

            // Calculate 'sin' value based on the distance
            main_ptr->sin = (unsigned)(SIDE * fabs(sin(2 * M_PI * d / SIDE)));

            // Initialize momentum and spin vectors
            main_ptr->p[0] = 0;
            main_ptr->p[1] = 0;
            main_ptr->p[2] = 0;
            main_ptr->s[0] = 0;
            main_ptr->s[1] = 0;
            main_ptr->s[2] = 0;

            // Net charges initialization
            main_ptr->net_c0 = 0;
            main_ptr->net_c1 = 0;
            main_ptr->net_c2 = 0;
            main_ptr->net_q = 0;
            main_ptr->net_w0 = 0;
            main_ptr->net_w1 = 0;

            // Copy the cell to the draft lattice
            *draft_ptr++ = *main_ptr++;
        }

        // Calculate the central index for all layers in the w dimension
        for (int w_layer = 0; w_layer < SIDE; w_layer++)
        {
            // Calculate the central index for the w_layer
            int center = w_layer * SIDE3 + (SIDE / 2) * SIDE2 + (SIDE / 2) * SIDE + (SIDE / 2);
            // Define the seed cell
        	partialInit(lattice_main + center);
        	partialInit(lattice_draft + center);
        	initSpin(lattice_main + center);
        	generate_momentum_set(lattice_main + center);
        	initSpin(lattice_draft + center);
        	generate_momentum_set(lattice_draft + center);
        }
    }

	/**
	 * Hub for initialization routines.
	 */
	void initSimulation()
	{
		voxels = (COLORREF *)malloc(SIDE3 * sizeof(COLORREF));
		lattice_main = (Cell *)malloc(SIDE3 * DEPTH * sizeof(Cell));
		lattice_draft = (Cell *)malloc(SIDE3 * DEPTH * sizeof(Cell));
		lattice_mirror = (Cell *)malloc(SIDE3 * DEPTH * sizeof(Cell));
		initLattice();
	}

}
