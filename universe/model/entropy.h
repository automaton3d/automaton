/*
 * entropy.h
 *
 *  Created on: 19 de nov. de 2024
 *      Author: Alexandre
 */

#ifndef MODEL_ENTROPY_H_
#define MODEL_ENTROPY_H_

#include <vector>
#include <unordered_map>
#include <cmath>

namespace automaton
{
	using namespace std;

    class Entropy
    {
    private:
        float maxEntropy;     // Maximum possible entropy for normalization
        vector<float> values; // Stores entropy values

    public:
        // Default constructor
        Entropy();

        // Getters
        int getCurrentIndex() const;
        int getWidth() const;
        float getMaxEntropy() const;
        int getPointer() { return values.size(); }

        // Setters with validation
        void setWidth(int w);
        void setHeight(int h);
        void setMaxEntropy(float m);

        // Add a new entropy value
        void add(float e);

        // Get the Y-coordinate for a given X based on entropy
        float getY(unsigned x) const;
    };

    class EntropyCalculator
    {
    private:
        Entropy entropy;
        std::unordered_map<unsigned, unsigned> stateCounts;
        unsigned totalStates;

    public:
        // Constructor
        EntropyCalculator();

        // Collect state data
        void collectData();

        // Compute entropy
        double computeEntropy();

        // Reset state counts
        void resetCounts();

        // Access entropy object for visualization
        const Entropy& getEntropy() const;
        void updateEntropy();
    };

} // namespace automaton

#endif /* MODEL_ENTROPY_H_ */
