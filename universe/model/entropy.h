/*
 * entropy.h
 *
 *  Created on: 19 de nov. de 2024
 *      Author: Alexandre
 */

#ifndef MODEL_ENTROPY_H_
#define MODEL_ENTROPY_H_

#include "simulation.h"

#define ENTROPIES 480

namespace automaton
{
	using namespace std;

	// Forward declaration of Cell
	class Cell;

	// The object encapsulating entropy

    class Entropy
    {
    private:
        float minEntropy;
        float maxEntropy;       // Maximum possible entropy for normalization
        vector<float> states;   // Stores CA states
        float entropies[ENTROPIES]; // Stores the calculated entropies
        int pointer = 0;        // Position of last entropy value

    public:
        // Default constructor
        Entropy();

        // Getters
        float getMinEntropy() const { return minEntropy; }
        float getMaxEntropy() const { return maxEntropy; }
        int getPointer() { return pointer; }
        float getY(unsigned x) const;

        // Setter
        void setMinEntropy(float m) { minEntropy = m; }
        void setMaxEntropy(float m) { maxEntropy = m; }

        // Add a new state value
        void addState(float state);

        // Add a new entropy value
        void addEntropy(float H);

        // Resets the graph bar collector
        void resetBars();

        bool isFull()
        {
           return pointer >= ENTROPIES;
        }
    };

    // The object encapsulating entropy calculation

    class EntropyCalculator
    {
    private:
        Entropy entropy;
        std::unordered_map<unsigned, unsigned> stateCounts;
        unsigned totalStates;

        /*
         * Resets the data collector.
         * Called by updateEntropy().
         */
        void resetCollector()
        {
          stateCounts.clear();
          totalStates = 0;
        }

    public:
        // Constructor
        EntropyCalculator();

        // Calculates the state of one cell
        uint32_t cellState(unsigned x, unsigned y, unsigned z, Cell *cell);

        // Collect state data
        void collectData();

        // Compute entropy
        double computeEntropy();


        // Access entropy object for visualization
        const Entropy& getEntropy() const;
        void updateEntropy();
    };

} // namespace automaton

#endif /* MODEL_ENTROPY_H_ */
