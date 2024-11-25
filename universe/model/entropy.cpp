/*
 * entropy.cpp
 *
 * See Section Entropy Considerations
 *
 *  Created on: 16 de nov. de 2024
 *      Author: Alexandre
 */

#include "simulation.h"

#define NON_ZERO_STATE_COUNT_LIMIT 11  // Adjust this based on your state's range

namespace automaton
{
    using namespace std;

    // Global variables (ensure they are initialized properly elsewhere in your code)
    unordered_map<unsigned, unsigned> nonZeroStateCounts; // Sparse map for non-zero states
    unsigned zeroStateCount = 0;  // Separate counter for zero states
    unsigned totalStates = 0;     // Total number of states processed across all calls

    Entropy entropy;
    double H0;	// Initial entropy

    /*
     * State function that selects specific properties of interest to represent the state of a cell.
     */
    uint32_t cellState(Cell *cell)
    {
        if (cell->pole)
        {
            uint32_t p_x = cell->pos[0] & ((1U << ORDER) - 1);
            uint32_t p_y = cell->pos[1] & ((1U << ORDER) - 1);
            uint32_t p_z = cell->pos[2] & ((1U << ORDER) - 1);
            return cell->charge
                 | (cell->aff << 6)
                 | (cell->freq << (6 + ORDER))
                 | (p_x << (6 + ORDER + (ORDER - 1)))
                 | (p_y << (6 + ORDER + (ORDER - 1) + ORDER))
                 | (p_z << (6 + ORDER + (ORDER - 1) + 2 * ORDER));
        }
        return 0;
    }

    // Function that collects and counts the state for a single cell position and depth
    void stateFunction(int x, int y, int z)
    {
        // Iterate over the depth dimension (W_DIM layers)
        for (unsigned w = 0; w < W_DIM; ++w)
        {
            // Calculate the index for the flattened 1D array using (x, y, z, w)
            int index = (x * SIDE2 + y * SIDE + z) + (w * SIDE3);
            Cell& cell = lattice_current[index];  // Access the current cell
            // Get the state of the cell
            unsigned state = cellState(&cell);  // Assuming cellState is a function returning the cell's state
            // Count zero state
            if (state == 0)
            {
                zeroStateCount++;  // Increment zero state counter
            }
            else
            {
                nonZeroStateCounts[state]++;  // Increment count for this specific non-zero state
            }
            // Increment the total state count
            totalStates++;
        }
    }


    /*
     * Collects data from all cells in the lattice and updates the global state counts.
     */
    void collectData()
    {
        for (int x = 0; x < SIDE; x++)
        {
            for (int y = 0; y < SIDE; y++)
            {
                for (int z = 0; z < SIDE; z++)
                {
                    stateFunction(x, y, z);
                }
            }
        }
    }

    /*
     * Computes Shannon entropy using the accumulated state counts.
     */
    double computeEntropy()
    {
        double H = 0;
        double probs = 0;
        entropy.setMaxEntropy(log2(totalStates)); // Set max entropy

        // Include zero state in entropy calculation
        if (zeroStateCount > 0)
        {
            double prob = zeroStateCount / (double)totalStates;
            probs += prob;
            H += prob * log2(prob);
        }

        // Compute probabilities and entropy for non-zero states
        for (const auto& par : nonZeroStateCounts)
        {
            double prob = par.second / (double)totalStates;
            probs += prob;
            H += prob * log2(prob);
        }

        // Guarantee unitarity
        assert(abs(probs - 1.0) <= 1e-9);

        return -H; // Entropy is negative sum
    }

    void detectPoincare()
    {
        // Test the completion of the Poincaré cycle graph
        if (framework::timer == POINCARE)
        {
    		// Play trout.wav
        	framework::sound();
        	// The program stops here - END
        }
        // Update entropy data
        if (framework::timer % FRAME == 0)
    	{
        	collectData();
    	}
        double H = 0;
        if (framework::timer % FRAME * ERA == 0)
        {
        	H = computeEntropy();
        }
       	if (H == H0 || rand() % 2 == 0)
       	{
       		printf("***** Poincaré=%ld *****\n", framework::timer);
       		// Play trout.wav
           	framework::sound();
           	// The program stops here - END
        }
    	else
    	{
    		printf("H=%f\n", H);	// debug
    		Beep(750, 300);
    		entropy.add(H);
    	}
    }

} // End of namespace automaton
