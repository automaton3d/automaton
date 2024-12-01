/*
 * entropy.cpp
 *
 * See Section Entropy Considerations
 */

#include "simulation.h"
#include "entropy.h"
#include "poincare.h"
#include "../mygl.h"

namespace framework
{
  extern unsigned long long timer;
}

namespace automaton
{
  double H0;
  extern unsigned era;
  extern unsigned long long poincare;//DEBUG

  // Implementation of Entropy class

  Entropy::Entropy() : maxEntropy(1.0f)
  {
  }

  float Entropy::getMaxEntropy() const { return maxEntropy; }

  void Entropy::setMaxEntropy(float m)
  {
    maxEntropy = m;
  }

  void Entropy::add(float H)
  {
    values.push_back(H);
    if (values.size() == WIDTH)
    {
      boostPoincare();
      values.clear();			// Wrap around
    }
  }

  float Entropy::getY(unsigned x) const
  {
	if (x >= values.size())
		return 0;
    return values[x];
  }

  // Implementation of EntropyCalculator class

  EntropyCalculator::EntropyCalculator() : totalStates(0)
  {
  }

  void EntropyCalculator::collectData()
  {
    stateCounts.clear();
    totalStates = 0;
    for (int x = 0; x < SIDE; ++x)
      for (int y = 0; y < SIDE; ++y)
        for (int z = 0; z < SIDE; ++z)
        {
          for (unsigned w = 0; w < W_DIM; ++w)
          {
            int index = x * SIDE2 + y * SIDE + z + w * SIDE3;
            Cell& cell = lattice_current[index];
            if (cell.pos[0] == 0 && cell.pos[1] == 0 && cell.pos[2] == 0)
            {
            	unsigned state = cellState(x, y, z, &cell);
           		++stateCounts[state];
            	++totalStates;
            }
          }
        }
  }

  double EntropyCalculator::computeEntropy()
  {
    double H = 0.0;
    assert(totalStates > 0);
    double maxH = log2(totalStates);
    entropy.setMaxEntropy(maxH);
    for (const auto& entry : stateCounts)
    {
      double prob = entry.second / static_cast<double>(totalStates);
      H += prob * log2(prob);
    }
    return -H;
  }

  void EntropyCalculator::resetCounts()
  {
    stateCounts.clear();
    totalStates = 0;
  }

  const Entropy& EntropyCalculator::getEntropy() const
  {
    return entropy;
  }

  void EntropyCalculator::updateEntropy()
  {
    // printf("timer=%llu ERA=%u\n", framework::timer, ERA); fflush(stdout); // Debug
    if (framework::timer % FRAME == 0)
    {
      collectData();
    }
    if (framework::timer % (FRAME * era) == 0)
    {
      double H = computeEntropy();
      printf("H=%f timer=%llu PC=%llu pointer=%d\n", H, framework::timer, poincare, entropy.getPointer()); fflush(stdout); // Debug
      entropy.add(H);
    }
  }

} // namespace automaton
