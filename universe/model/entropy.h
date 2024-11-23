/*
 * entropy.h
 *
 *  Created on: 19 de nov. de 2024
 *      Author: Alexandre
 */

#ifndef MODEL_ENTROPY_H_
#define MODEL_ENTROPY_H_

#include <vector>
#include <cstdlib>

namespace automaton
{

	class Entropy
	{
		int width;              // Width of the graph
		int index;              // Current index in the values array
		int height;        // Height of the graph in pixels
		float maxEntropy;       // Maximum possible entropy for normalization
		std::vector<float> values; // Stores entropy values

	public:

		// Default constructor
		Entropy() :
			width(480),
			index(0),
			height(200),
			maxEntropy(10.0f),
			values(480, 0.0f)
		{
		}

		// Set the height of the graph
		void setWidth(int w)
		{
			width = w;
		}

		// Set the height of the graph
		void setHeight(int h)
		{
			height = h;
		}

		// Set the maximum entropy value
		void setMaxEntropy(float m)
		{
			maxEntropy = m;
		}

		// Add a new entropy value
		void add(float e)
		{
			values[index++] = e;
			if (index == width) // Wrap around when index reaches the width
				index = 0;
		}

		// Get the Y-coordinate for a given X based on entropy
		int getY(int x) const
		{
			if (x < 0 || x >= width) // Ensure x is within bounds
				return 0;

			return static_cast<int>(values[x] * height / maxEntropy);
		}
	};

	// Function declarations for entropy-related computations
	void collectData();
	double computeEntropy();

} // namespace automaton

#endif /* MODEL_ENTROPY_H_ */
