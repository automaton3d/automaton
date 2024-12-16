/*
 * nvector.h
 *
 *  Created on: 14 de dez. de 2024
 *      Author: Alexandre
 */

#ifndef MODEL_NVECTOR_H_
#define MODEL_NVECTOR_H_

#include <iostream>

namespace automaton
{
  class NVector
  {
    public:
      unsigned x, y, z;
      bool sign;

      // Default constructor
      NVector() : x(0), y(0), z(0), sign(false) {}

      // Constructor without sign parameter (default to true)
      NVector(unsigned x, unsigned y, unsigned z)
        : x(x), y(y), z(z), sign(false) {}

      // Constructor with all parameters
      NVector(unsigned x, unsigned y, unsigned z, bool sign)
        : x(x), y(y), z(z), sign(sign) {}

      // Assignment operator
      NVector& operator=(const NVector& other)
      {
        if (this != &other)
        {
          x = other.x;
          y = other.y;
          z = other.z;
          sign = other.sign;
        }
        return *this;
    }

    // Equality operator
    bool operator==(const NVector& other) const
    {
      return (x == other.x && y == other.y && z == other.z && sign == other.sign);
    }

    // Inequality operator
    bool operator!=(const NVector& other) const
    {
      return !(*this == other);
    }

    // Print function for debugging
    void print() const
    {
      std::cout << "Vector(" << x << ", " << y << ", " << z << "), Sign: " << (sign ? "Positive" : "Negative") << std::endl;
    }
  };

}
#endif /* MODEL_NVECTOR_H_ */
