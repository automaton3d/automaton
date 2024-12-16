/*
 * io.cpp
 *
 *  Created on: 24 de nov. de 2024
 *      Author: Alexandre
 */
#include "../GUIrenderer.h"
#include "simulation.h"

namespace framework
{
  extern bool poincare;
}

namespace automaton
{
  using namespace std;

  EntropyCalculator entropyCalc;

  unsigned long long poincare;
  unsigned era;

  /**
   * Initializes the Poincaré cycle with a reasonable value.
   */
  void initPoincare()
  {
    era = 2;
    poincare = WIDTH * era;
  }

  /**
   * Increases the estimate of the Poincaré cycle by a certain amount.
   */
  void boostPoincare()
  {
    era++;
    poincare = WIDTH * era;
  }

  /**
   * Detects if the simulation completed a Poincaré cycle.
   */
  void detectPoincare()
  {
    // Disregard initial state time trap
    if (framework::timer < FRAME)
      return;
    // First hint:
    // Check if tick clocks are ok
    if ((lattice_curr[0][0][0][0].t || lattice_curr[0][0][0][0].k))
      return;
    // puts("passed hint 1");
    // Second hint:
    // All bubbles must be superposed at the center
    for (unsigned w = 0; w < W_DIM; ++w)
    {
      // Calculate the index for the center cell
      Cell &cell = lattice_curr[CENTER][CENTER][CENTER][w];  // Access the current cell
      if (cell.pos[0] != CENTER || cell.pos[1] != CENTER || cell.pos[2] != CENTER)
        return;
    }
    // puts("passed hint 2");
    // Third hint:
    // Check if the entropy is the same as the first registered
    entropyCalc.collectData();
    double H = entropyCalc.computeEntropy();
    if (H != entropyCalc.getEntropy().getMinEntropy())
      return;
    // puts("passed hint 3");
    // Final check:
    // Perform an exhaustive, time consuming, evaluation.
    // The data is stored on disk
    if (!checkPoincare())
      return;
    puts("passed all");
    //
    // Passed all tests:
    //
    // Open alert window
    framework::poincare = true;
    // Play trout.wav
    framework::sound(true);
    // The program stops here - END
    while(true)
      Sleep(1000);
  }

  /////////////// Internal stuff ///////////////////

  // Serialize SN
  void serializeSN(const SN& sn, ofstream& out)
  {
    if (!out)
      throw ios_base::failure("Failed to open output stream.");
    out.write(reinterpret_cast<const char*>(&sn.a), sizeof(sn.a));
    out.write(reinterpret_cast<const char*>(&sn.s), sizeof(sn.s));
  }

  // Deserialize SN
  void deserializeSN(SN& sn, ifstream& in)
  {
    if (!in)
      throw ios_base::failure("Failed to open input stream.");
    in.read(reinterpret_cast<char*>(&sn.a), sizeof(sn.a));
    in.read(reinterpret_cast<char*>(&sn.s), sizeof(sn.s));
  }

  // Serialize Cell
  void Cell::serialize(ofstream& out) const
  {
    if (!out) throw ios_base::failure("Failed to open output stream.");

    out.write(reinterpret_cast<const char*>(&pole), sizeof(pole));
    out.write(reinterpret_cast<const char*>(&charge), sizeof(charge));
    out.write(reinterpret_cast<const char*>(s), sizeof(s));
    out.write(reinterpret_cast<const char*>(&aff), sizeof(aff));
    out.write(reinterpret_cast<const char*>(pos), sizeof(pos));

    out.write(reinterpret_cast<const char*>(&wv), sizeof(wv));
    out.write(reinterpret_cast<const char*>(&d), sizeof(d));
    out.write(reinterpret_cast<const char*>(&sin), sizeof(sin));
    serializeSN(A, out);
    out.write(reinterpret_cast<const char*>(&freq), sizeof(freq));
    out.write(reinterpret_cast<const char*>(&angle), sizeof(angle));

    out.write(reinterpret_cast<const char*>(c), sizeof(c));
    out.write(reinterpret_cast<const char*>(&k), sizeof(k));
    out.write(reinterpret_cast<const char*>(&t), sizeof(t));

    out.write(reinterpret_cast<const char*>(m), sizeof(m));
    out.write(reinterpret_cast<const char*>(&e), sizeof(e));
    serializeSN(A_bar, out);
    serializeSN(ph, out);

    out.write(reinterpret_cast<const char*>(&collapse), sizeof(collapse));
    out.write(reinterpret_cast<const char*>(&net_c0), sizeof(net_c0));
    out.write(reinterpret_cast<const char*>(&net_c1), sizeof(net_c1));
    out.write(reinterpret_cast<const char*>(&net_c2), sizeof(net_c2));
    out.write(reinterpret_cast<const char*>(&net_q), sizeof(net_q));
    out.write(reinterpret_cast<const char*>(&net_w0), sizeof(net_w0));
    out.write(reinterpret_cast<const char*>(&net_w1), sizeof(net_w1));
    out.write(reinterpret_cast<const char*>(&fxf), sizeof(fxf));
    out.write(reinterpret_cast<const char*>(&bxb), sizeof(bxb));
    out.write(reinterpret_cast<const char*>(&fxb), sizeof(fxb));
    out.write(reinterpret_cast<const char*>(&wxw), sizeof(wxw));
    out.write(reinterpret_cast<const char*>(&boson), sizeof(boson));
  }

  // Deserialize Cell
  void Cell::deserialize(ifstream& in)
  {
    if (!in)
      throw ios_base::failure("Failed to open input stream.");
    in.read(reinterpret_cast<char*>(&pole), sizeof(pole));
    in.read(reinterpret_cast<char*>(&charge), sizeof(charge));
    in.read(reinterpret_cast<char*>(s), sizeof(s));
    in.read(reinterpret_cast<char*>(&aff), sizeof(aff));
    in.read(reinterpret_cast<char*>(pos), sizeof(pos));
    in.read(reinterpret_cast<char*>(&wv), sizeof(wv));
    in.read(reinterpret_cast<char*>(&d), sizeof(d));
    in.read(reinterpret_cast<char*>(&sin), sizeof(sin));
    deserializeSN(A, in);
    in.read(reinterpret_cast<char*>(&freq), sizeof(freq));
    in.read(reinterpret_cast<char*>(&angle), sizeof(angle));
    in.read(reinterpret_cast<char*>(c), sizeof(c));
    in.read(reinterpret_cast<char*>(&k), sizeof(k));
    in.read(reinterpret_cast<char*>(&t), sizeof(t));
    in.read(reinterpret_cast<char*>(m), sizeof(m));
    in.read(reinterpret_cast<char*>(&e), sizeof(e));
    deserializeSN(A_bar, in);
    deserializeSN(ph, in);
    in.read(reinterpret_cast<char*>(&collapse), sizeof(collapse));
    in.read(reinterpret_cast<char*>(&net_c0), sizeof(net_c0));
    in.read(reinterpret_cast<char*>(&net_c1), sizeof(net_c1));
    in.read(reinterpret_cast<char*>(&net_c2), sizeof(net_c2));
    in.read(reinterpret_cast<char*>(&net_q), sizeof(net_q));
    in.read(reinterpret_cast<char*>(&net_w0), sizeof(net_w0));
    in.read(reinterpret_cast<char*>(&net_w1), sizeof(net_w1));
    in.read(reinterpret_cast<char*>(&fxf), sizeof(fxf));
    in.read(reinterpret_cast<char*>(&bxb), sizeof(bxb));
    in.read(reinterpret_cast<char*>(&fxb), sizeof(fxb));
    in.read(reinterpret_cast<char*>(&wxw), sizeof(wxw));
    in.read(reinterpret_cast<char*>(&boson), sizeof(boson));
  }

  /*
   * Save the current state of the CA to disk.
   */
  void saveState0()
  {
      ofstream out("CA-state0.dat", ios::binary);
      if (!out)
      {
          cerr << "Error opening file for writing!" << endl;
          return;
      }
      // Save the total number of objects
      size_t count = BLOCK;
      out.write(reinterpret_cast<const char*>(&count), sizeof(count));
      // Save each Cell object in the lattice
      for (int x = 0; x < SIDE; x++)
          for (int y = 0; y < SIDE; y++)
        	  for (int z = 0; z < SIDE; z++)
        		  for (int w = 0; w < W_DIM; w++)
        			  lattice_curr[x][y][z][w].serialize(out);
      out.close();
  }

  bool compareCells(const Cell& cell1, const Cell& cell2)
  {
    // Compare scalar members
    if (cell1.pole != cell2.pole || cell1.charge != cell2.charge || cell1.aff != cell2.aff)
    {
      return false;
    }
    // Compare arrays (if s, pos, m, etc., are arrays or containers)
    if (memcmp(cell1.s, cell2.s, sizeof(cell1.s)) != 0 ||
        memcmp(cell1.pos, cell2.pos, sizeof(cell1.pos)) != 0 ||
        memcmp(cell1.m, cell2.m, sizeof(cell1.m)) != 0)
    {
      return false;
    }
    // Add other comparisons as needed
    return true; // All comparisons passed
  }

  bool checkPoincare()
  {
      //#define DEBUG

  #ifdef DEBUG
      // For debug purposes, occasionally return true randomly
      if (rand() % 16 == 0)
      {
          return true;
      }
  #endif

      ifstream in("CA-state0.dat", ios::binary); // Open file in binary mode
      if (!in)
      {
          cerr << "Error opening file for reading!" << endl;
          return false;
      }

      size_t count;
      in.read(reinterpret_cast<char*>(&count), sizeof(count)); // Read number of objects

      if (count != BLOCK) // Validate size consistency
      {
          cerr << "Mismatch in lattice size: expected " << BLOCK << ", found " << count << endl;
          in.close();
          return false;
      }

      Cell saved_cell;
      bool success = true;

      // Iterate over the array and compare each cell
      for (int x = 0; x < SIDE; x++)
          for (int y = 0; y < SIDE; y++)
        	  for (int z = 0; z < SIDE; z++)
        		  for (int w = 0; w < W_DIM; w++)
        		  {
        			  saved_cell.deserialize(in); // Deserialize cell data from file
        			  if (!compareCells(lattice_curr[x][y][z][w], saved_cell))
        			  {
        				  success = false;
        			  }
        		  }
      in.close();
      return success;
  }

}
