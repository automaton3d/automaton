/*
 * entropy.cpp
 *
 * Implementation of Entropy and EntropyCalculator classes.
 * (See Section Entropy Considerations)
 */

#include "simulation.h"
#include "entropy.h"
#include "poincare.h"

namespace framework
{
  extern unsigned long long timer;
}

namespace automaton
{
  extern unsigned era;
  extern unsigned long long poincare;//DEBUG

  ////// Implementation of the Entropy class /////////

  /**
   * Default constructor.
   */
  Entropy::Entropy() : minEntropy(0), maxEntropy(1.0f)
  {
  }

  /**
   * Adds an state value to the calculator.
   *
   * @H the entropy value
   */
  void Entropy::addState(float H)
  {
    states.push_back(H);
  }

  /**
   * Adds a new entropy value.
   */
  void Entropy::addEntropy(float H)
  {
    assert(pointer < ENTROPIES);
    entropies[pointer++] = H;
  }

  /**
   * Retrieves the indexed entropy value.
   *
   * @x the position index
   */
  float Entropy::getY(unsigned x) const
  {
    assert(x < ENTROPIES && "zebra!");
    return entropies[x];
  }

  /*
   * Resets the graph bar collector.
   */
  void Entropy::resetBars()
  {
    pointer = 0;
  }

  ////// Implementation of the EntropyCalculator class /////////

  /**
   * Default constructor.
   */
  EntropyCalculator::EntropyCalculator() : totalStates(0)
  {
  }

  /**
   * Used to collect data after each light step.
   * It counts the states in the present configuration.
   */
  void EntropyCalculator::collectData()
  {
    stateCounts.clear();
    totalStates = 0;
    for (unsigned w = 0; w < W_DIM; ++w)
    {
      // Flag to break outer loops when central cell is found
      bool found = false;
      int x, y, z;
      for (x = 0; x < SIDE; ++x)
      {
        for (y = 0; y < SIDE; ++y)
        {
          for (z = 0; z < SIDE; ++z)
          {
            Cell &cell = lattice_curr[x][y][z][w];
            // Locate the central cell in the 3D lattice
            if (cell.pos[0] == CENTER && cell.pos[1] == CENTER && cell.pos[2] == CENTER)
            {
              // Calculate the cell state.
              // This cell is representative of the entire 3D lattice,
              // compute its state.
              unsigned state = cellState(x, y, z, &cell);
              // Count the state
              ++stateCounts[state];
              ++totalStates;
              // Set the flag to stop the loops after finding the central cell
              found = true;
              break;  // Exit the innermost loop
            }
          }
          // Exit the outer loops if central cell has been found
          if (found)
            break;
        }
        // Exit the outer loops if central cell has been found
        if (found)
          break;
      }
    }
    //assert(totalStates == W_DIM); Ignorar, por enquanto TODO
  }

  /**
   * State function that selects specific properties of interest to
   * represent the state of a cell.
   *
   * @x,y,z the offset of the cell
   * @cell pointer to the calculated cell
   */
  uint32_t EntropyCalculator::cellState(unsigned x, unsigned y, unsigned z, Cell *cell)
  {
    // Cocatenate the charge and offset bits
    return cell->charge
        | (x << (6 + 1*ORDER))
        | (y << (6 + 2*ORDER))
        | (z << (6 + 3*ORDER));
  }

  /**
   * Calculates the entropy for the configurations collected
   * during the last era.
   */
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

  /**
   * Gets the entropy object.
   */
  const Entropy& EntropyCalculator::getEntropy() const
  {
    return entropy;
  }

  /**
   * High level routine that coordinates the data collection
   * and entropy calculation.
   */
  void EntropyCalculator::updateEntropy()
  {
    if (framework::timer % FRAME == 0)
    {
      //assert(sanityTest1());
      //assert(sanityTest2());
      //assert(sanityTest3());
    //  assert(sanityTest4());
      collectData();
    }
    if (framework::timer % (FRAME * era) == 0)
    {
      double H = computeEntropy();
      printf("H=%f timer=%llu PC=%llu\n", H, framework::timer, poincare); fflush(stdout); // Debug
      resetCollector();
      // Collect the entropy value
      entropy.addEntropy(H);
      if (entropy.isFull())
      {
        entropy.resetBars();
        boostPoincare();
      }
    }
  }

} // namespace automaton


