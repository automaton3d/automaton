/*
 * tests.cpp
 *
 *  Created on: 2 de dez. de 2024
 *      Author: Alexandre
 */
#include "simulation.h"

namespace automaton
{
  // Função para imprimir os valores das constantes
  void printConstants()
  {
    cout << "SIDE =\t\t" << SIDE << endl;
    cout << "SIDE2 =\t\t" << SIDE2 << endl;
    cout << "SIDE3 =\t\t" << SIDE3 << endl;
    cout << "3*SIDE =\t" << 3*SIDE << endl;
    cout << "W_DIM =\t\t" << W_DIM << endl;
    cout << "CENTER =\t" << CENTER << endl;
    cout << "DIAG =\t\t" << DIAG << endl;
    cout << "RMAX =\t\t" << RMAX << endl;
    cout << "FMAX =\t\t" << FMAX << endl;
    cout << "CONVOL =\t" << CONVOL << " (k)"  << endl;
    cout << "COLLISION =\t"    << COLLISION << " (k)"  << endl;
    cout << "DIFFUSION =\t"  << DIFFUSION << " (k)"  << endl;
    cout << "RELOCATION =\t\t" << RELOCATION << " (k)"  << endl;
    cout << "TRANSPORT =\t\t"  << TRANSPORT << " (k)"  << endl;
    cout << "UPDATE =\t" << UPDATE << " (k)"  << endl;
    cout << "LIGHT =\t\t" << LIGHT << " (t)" << endl;
    cout << "RANGE =\t\t" << RANGE << " (t)"  << endl;
    cout << "FRAME =\t\t" << FRAME << " (k)"  << endl;
    cout << "BLOCK =\t\t" << BLOCK << endl;
  }

  void testReloc(Cell& curr, Cell &draft, Cell &mirror, unsigned w)
  {
      if (framework::timer == 1044 && curr.wv && w == 0 && curr.pos[0] == 3 && curr.pos[1] == 3 && curr.pos[2] == 3)
      {
    	printf("%llu %u,%u,%u\n", framework::timer, curr.pos[0], curr.pos[1], curr.pos[2]); fflush(stdout);
        draft.fxf = true;
        // Master look-ahead relocate his own pole
        draft.c[0] = SIDE / 4;
        draft.c[1] = SIDE / 4;
        draft.c[2] = SIDE / 4;
      }
  }


  /**
   * Checks if all layers contain exactly one central
   * cell (pos=(CENTER,CENTER,CENTER)).
   */
  bool sanityTest1()
  {
    unsigned shellCounts[W_DIM] = {0}; // Array to count the central cells in each shell
    // Traverse all shells (w-dimension layers)
    for (unsigned w = 0; w < W_DIM; ++w)
    {
      for (unsigned x = 0; x < SIDE; ++x)
      {
        for (unsigned y = 0; y < SIDE; ++y)
        {
          for (unsigned z = 0; z < SIDE; ++z)
          {
            // Reference to the current cell
            Cell& cell = lattice_curr[x][y][z][w];
            // Check if the cell is central
            if (cell.pos[0] == CENTER && cell.pos[1] == CENTER && cell.pos[2] == CENTER)
              shellCounts[w]++;
          }
        }
      }
    }
    // Check if every shell has exactly one central cell
    bool isValid = true;
    for (unsigned w = 0; w < W_DIM; ++w)
    {
      if (shellCounts[w] != 1)
      {
        isValid = false;
        printf("Sanity 1 failed: shell %u has %u central cells (expected 1).\n", w, shellCounts[w]); fflush(stdout);
        break;
      }
    }
    return isValid;
  }

  /**
   * Cheks if all cells in a layer have the same charge.
   */
  bool sanityTest2()
  {
    bool isValid = true;
    // Sweep each layer
    for (unsigned w = 0; w < W_DIM; ++w)
    {
      // Reference charge
      Cell& cell0 = lattice_curr[0][0][0][w];
      // Traverse all cells in the current shell (w-dimension layer)
      for (unsigned x = 0; x < SIDE; ++x)
      {
        for (unsigned y = 0; y < SIDE; ++y)
        {
          for (unsigned z = 0; z < SIDE; ++z)
          {
            Cell& cell = lattice_curr[x][y][z][w];
            if (cell.charge != cell0.charge)
            {
              isValid = false;
              break;
            }
          }
        }
      }
      if (!isValid)
      {
        printf("T2 failed on w=%u (charges differ)", w);
        break;
      }
    }
    return isValid;
  }

  /**
   * Checks if all cells in each layer have the same c[] values.
   */
  bool sanityTest3()
  {unsigned c0 = 0;
    bool isValid = true;
    // Sweep each layer
    for (unsigned w = 0; w < W_DIM; ++w)
    {
      Cell& cell0 = lattice_curr[0][0][0][w];


      if (w == 0)
    	  c0 = cell0.c[0];

      unsigned count = 0;
      for (unsigned x = 0; x < SIDE; ++x)
      {
        for (unsigned y = 0; y < SIDE; ++y)
        {
          for (unsigned z = 0; z < SIDE; ++z)
          {
            // Reference to the current cell
            Cell& cell = lattice_curr[x][y][z][w];
            if(cell.c[0] == cell0.c[0] &&
               cell.c[1] == cell0.c[1] &&
		       cell.c[2] == cell0.c[2])
              count++;
          }
        }
      }
      if (count != SIDE3)
      {
        printf("T3 failed (c[]'s differ) %u != %u\n", count, SIDE3);
        isValid = false;
        break;
      }
      else //DEBUG
      {
    	  if (c0)
    	  {
    		  printf("c0 = %u\n", c0); fflush(stdout);
    	  }
      }
    }
    return isValid;
  }

  /**
   * Is the Euclidean distance checksum the same in all layers?.
   */
  bool sanityTest4()
  {
    unsigned checksum = 0;
    bool isValid = true;
    // Sweep each layer
    for (unsigned w = 0; w < W_DIM; ++w)
    {
      unsigned sum = 0;
      // Traverse all cells in the current shell (w-dimension layer)
      for (unsigned x = 0; x < SIDE; ++x)
      {
        for (unsigned y = 0; y < SIDE; ++y)
        {
          for (unsigned z = 0; z < SIDE; ++z)
          {
            // Reference to the current cell
            Cell& cell = lattice_curr[x][y][z][w];
            sum += cell.d;
          }
        }
      }
      if (w == 0)
        checksum = sum;
      if (checksum != sum)
      {
        isValid = false;
        printf("T4 failed (d checksums differ in w=%u)\n", w);
        fflush(stdout);
        break;
      }
      if (!isValid)
        break;
    }
    return isValid;
  }

  /*
   * Check if all cells have the same k value.
   */
  bool sanityTest5()
  {
    unsigned k_ref = lattice_curr[0][0][0][0].k;
    for (unsigned w = 0; w < W_DIM; ++w)
	{
	  for (unsigned x = 0; x < SIDE; ++x)
	  {
	    for (unsigned y = 0; y < SIDE; ++y)
	    {
	      for (unsigned z = 0; z < SIDE; ++z)
	      {
	        Cell& cell = lattice_curr[x][y][z][w];
	        if (cell.k != k_ref)
	          return false;
	      }
	    }
	  }
	}
    return true;
  }

  /**
   * Helper function.
   */
  void displayLattice()
  {
    system("cls");
    // Cabeçalho da grade
    for (int i = 0; i < SIDE; i++)
      printf("L:%d\t\t\t", i);
    printf("\n");

    //printf("w: %d\n", w); // Imprime o valor da dimensão w
    // Iteração sobre as linhas y
    for (int y = 0; y < SIDE; y++)
    {
      // Iteração sobre as camadas z
      for (int z = 0; z < SIDE; z++)
      {
        // Iteração sobre as colunas x
        for (int x = 0; x < SIDE; x++)
        {
          /* TODO
          printf("[%3u,%d]", (lattice_current + index3D)->d, (lattice_current + index3D)->m);
          if (lattice_current[index3D].wv)
            printf("X ");
          else
            printf(". ");
          */
        }
        cout << "   ";  // Adiciona espaço entre as camadas
      }
      cout << endl;  // Move para a próxima linha após todas as camadas
    }
    puts(" "); // Espaço entre as iterações de w
    Sleep(50);  // Ajuste o tempo de pausa se necessário
  }

}
