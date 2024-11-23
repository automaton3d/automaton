/*
 * entropy.cpp
 *
 * See Section Entropy Considerations
 *
 *  Created on: 16 de nov. de 2024
 *      Author: Alexandre
 */

#include "simulation.h"

namespace automaton
{
    using namespace std;

    unordered_map<unsigned, unsigned> nonZeroStateCounts; // Sparse map for non-zero states
    unsigned zeroStateCount = 0;	// Separate counter for zero states
    unsigned totalStates = 0;		// Total number of states processed across all calls

    Entropy entropy;

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

    /*
     * State function that collects and counts the state for a single cell position and depth.
     */
    void stateFunction(int x, int y, int z)
    {
        Cell *ptr = lattice_current + x * SIDE2 + y * SIDE + z;
        for (unsigned w = 0; w < W_DIM; w++, ptr += SIDE3)
        {
            unsigned state = cellState(ptr);
            if (state == 0)
            {
                zeroStateCount++;  // Increment zero state counter
            }
            else
            {
                nonZeroStateCounts[state]++;  // Increment count for non-zero states
            }
            totalStates++; // Track total number of states
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
    	entropy.setMaxEntropy(log2(totalStates));
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
//        H=-rand()/ (double)RAND_MAX;
        return -H; // Entropy is negative sum
    }

}
