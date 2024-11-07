/*
 * utils.cpp
 *
 * Ancillary code.
 */

#include "simulation.h"
#include "mygl.h"

namespace automaton
{
	unsigned last_n = 0;
	extern Cell *lattice_main;
	COLORREF *voxels;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(0, 99);

	int w = 0;  // Global variable for the w shell to process

	/**
	 * This is a bridge between the model and the graphical framework.
	 */
	void updateBuffer()
	{
	    if (!framework::active || framework::checkboxes.empty())
	        return;
	    for (int i = 0; i < LAYERS; i++)
	    {
	    	if (framework::layers[i].getState())
	    	{
	    		w = i;
	    	}
	    }

	    Cell *cell = lattice_main + SIDE3 * w;
	    for (int x = 0; x < SIDE; x++)
	    {
	        for (int y = 0; y < SIDE; y++)
	        {
	            for (int z = 0; z < SIDE; z++)
	            {
                    int index3D = x * SIDE2 + y * SIDE + z;
	                // Map phase to gray tones from THRESH to 255
	                unsigned tone = AMPLITUDE * cell->amplitude / SIDE + THRESH;

	                // Set voxel color based on the cell's wv property
	                voxels[index3D] = (cell->wv) ? RGB(tone, tone, tone) : RGB(0, 0, 0);
	                cell++;
	            }
	        }
	    }
	}

	void displayLattice()
    {
		system("cls");
		printf("DIFFUSE=%d\tSHIFTS=%d\tRMAX=%d\tUNIQUE=%d\tstep=%d\n", XYZ_DIFFUSION, RELOC, RMAX, LIGHT, lattice_main[0].n);

        // Cabeçalho da grade
        for (int i = 0; i < SIDE; i++)
            printf("L:%d\t\t\t", i);
        printf("\n");

        printf("w: %d\n", w); // Imprime o valor da dimensão w
        // Iteração sobre as linhas y
        for (int y = 0; y < SIDE; y++)
        {
            // Iteração sobre as camadas z
            for (int z = 0; z < SIDE; z++)
            {
                // Iteração sobre as colunas x
                for (int x = 0; x < SIDE; x++)
                {
                    // Cálculo direto do índice
                    int index3D = x * SIDE * SIDE + y * SIDE + z;
//#define PATTERN
#ifdef PATTERN
                    printf("[%3u,%d]", (lattice_main + index3D)->d, (lattice_main + index3D)->m);
#else
                    if ((lattice_main + index3D)->wv)
                        printf("+ ");
                    else
                        printf(". ");
#endif
                }
                std::cout << "   ";  // Adiciona espaço entre as camadas
            }
            std::cout << std::endl;  // Move para a próxima linha após todas as camadas
        }
        puts(" "); // Espaço entre as iterações de w
        Sleep(50);  // Ajuste o tempo de pausa se necessário
    }

	// Função para obter vizinhos considerando fronteiras toroidais
	Cell* get_neighbor(int index, int dir)
	{
	    int x = (index / SIDE3) % SIDE;
	    int y = (index / SIDE2) % SIDE;
	    int z = (index / SIDE) % SIDE;
	    int shell = index % SIDE;

	    switch (dir)
	    {
	        case NORTH: return lattice_main + (((x - 1 + SIDE) % SIDE) * SIDE3 + y * SIDE2 + z * SIDE + shell);
	        case EAST:  return lattice_main + (x * SIDE3 + ((y + 1) % SIDE) * SIDE2 + z * SIDE + shell);
	        case SOUTH: return lattice_main + (((x + 1) % SIDE) * SIDE3 + y * SIDE2 + z * SIDE + shell);
	        case WEST:  return lattice_main + (x * SIDE3 + ((y - 1 + SIDE) % SIDE) * SIDE2 + z * SIDE + shell);
	        case UP:    return lattice_main + (x * SIDE3 + y * SIDE2 + ((z + 1) % SIDE) * SIDE + shell);
	        case DOWN:  return lattice_main + (x * SIDE3 + y * SIDE2 + ((z - 1 + SIDE) % SIDE) * SIDE + shell);
	        default: return NULL;
	    }
	}

	bool isColorNeutral(unsigned char c1, unsigned char c2)
	{
	    unsigned res = (c1 ^ c2) & 7;
	    return ((res == 0 || res == 7) && c1 && c2 && c1 != 7 && c2 != 7);
	}

	bool isWeakNeutral(unsigned char c1, unsigned char c2)
	{
	    unsigned res = (c1 ^ c2) & 3; // Use 3 (binary 11) for 2 bits
	    return ((res == 0 || res == 3) && c1 != 0 && c2 != 0 && c1 != 3 && c2 != 3);
	}

	void printLattice()
	{
	    for (int w = 0; w < SIDE; w++)
	    {
	        // Calculate the central index as done in `initLattice`
	        int center = w * SIDE3 + (SIDE / 2) * SIDE2 + (SIDE / 2) * SIDE + (SIDE / 2);

	        Cell *pntr = lattice_main + center;
	        int p_magnitude = (int)round(sqrt(pntr->p[0] * pntr->p[0] + pntr->p[1] * pntr->p[1] + pntr->p[2] * pntr->p[2]));
	        int spin_magnitude = (int)round(sqrt(pntr->s[0] * pntr->s[0] + pntr->s[1] * pntr->s[1] + pntr->s[2] * pntr->s[2]));

	        printf("w=%d: p=(%d, %d, %d), |p|=%d\t\tspin=(%d, %d, %d), |spin|=%d\n",
	               w, pntr->p[0], pntr->p[1], pntr->p[2], p_magnitude,
	               pntr->s[0], pntr->s[1], pntr->s[2], spin_magnitude);
	    }
	}

}

#ifdef REBOTALHO

/*
 * Tests if the two given vectors are aligned.
 */
bool aligned(short* a, short* b)
{
  for (int i = 0; i < 3; i++)
  {
    if ((a[i] == 0 && b[i] != 0) || (a[i] != 0 && b[i] == 0))
      return false;
    if (a[i] * b[(i + 1) % 3] != a[(i + 1) % 3] * b[i])
      return false;
    if (a[i] * b[i] < 0)
      return false;
  }
  return true;
}

#define ZERO(v)      (!(v[0]|v[1]|v[2]))
#define EQ(v,u)      (v[0]==u[0]&&v[1]==u[1]&&v[2]==u[2])
#define ANTI(v,u)    (v[0]==-u[0]&&v[1]==-u[1]&&v[2]==-u[2])
#define ISSAT(v)     (v[0]==SIDE&&v[1]==SIDE&&v[2]==SIDE)
#define C(u)         (u->ch&C_MASK)      // color
#define _C(u)        ((~u->ch&C_MASK)&7) // anticolor
#define W1(u)        ((u->ch&W1_MASK)==W1_MASK)
#define W0(u)        ((u->ch&W0_MASK)==W0_MASK)
#define Q(u)         ((u->ch&Q_MASK)==Q_MASK)
#define MAT(u)       (C(u)>2&&C(u)!=4)
#define CMPL(u,v)    ((((~u)^W1_MASK)&0x3f)==v)
#define BUSY(c)      (c->k>COLLAPSE)
#define ANNIHIL(u,v) (C(u)==_C(v))


/**
 * Detects interactions.
 */
    // Test for same or different sectors.

    if (W1(nxt) == W1(drf))
    {
      // Same-sector.

      switch(code)
      {
        case 2:case 4:    // Weak interaction
          // puts("weak");
          // Particles are different.
          // Virtual photon capture.
          // Part of electrical, magnetic or
          // interference interactions.

          if (stb->k == PHOTON && DOT(nxt->m, stb->p) == 1)
          {
            // Recruit it. Draft is now a propeller.

            drf->a1 = nxt->a1;
          }
          else if (nxt->k == PHOTON &&
                   DOT(nxt->m, stb->p) == 1)
          {
            // Recruit it. Last is now a propeller.

            lst->a1 = stb->a1;
          }

          // Static forces.

          else if (drf->k == FERMION && !ZERO(nxt->p))
          {
            // Electric radial force.

            if (Q(nxt) == Q(stb))
            {
              // Use tick parity to distinguish E or M case.

              if (stb->n % SIDE_2 == 0)
              {
                // Repulsion.
                // drf is now a messenger.
                // (Sect. 5.6.1)

                SUB(lst->m, stb->o, nxt->o);
              }
              else
              {
                // Magnetic lateral kick.
                // drf is now a messenger.
                // (Sect. 5.6.2)

                CROSS(lst->m, lst->m, nxt->s);
              }
            }
            else
            {
              // Use tick parity to distinguish E or M case.

              if (stb->n % SIDE_2 == 0)
              {
                // Attraction.
                // drf is now a messenger.
                // (Sect. 5.6.1)

                SUB(lst->m, nxt->o, drf->o);
              }
              else
              {
                // Magnetic lateral kick.
                // drf is now a messenger.
                // (Sect. 5.6.2)

                CROSS(lst->m, lst->m, nxt->s);
              }
            }
          }

          // boson x boson.

          else if (nxt->k > FERMION && stb->k > FERMION)
          {
            // Exchange charges and spins.

            drf->ch &= (C_MASK | W0_MASK);
            drf->ch |= (nxt->ch & (C_MASK | W0_MASK));
            CP(drf->s, nxt->s);
            CP(drf->po, stb->p);

            // Reciprocity.

            lst->ch &= (C_MASK | W0_MASK);
            lst->ch |= (stb->ch & (C_MASK | W0_MASK));
            CP(lst->po, nxt->p);
            CP(lst->s, stb->s);
          }
          break;

        case 3:     // Inertia

          puts("inertia 1");
          // Calculate parallel transported pole.
          // bubbles have the same a1 value.
          // nxt is the master, stb is the slave.

          SUB(drf->po, nxt->o, stb->o);
          break;

        case 5:    // Inertia
          // puts("inertia 2");

          // Counterpart.
          // stb is the master, nxt is the slave.

          SUB(lst->po, stb->o, nxt->o);
          break;

        case 6:    // Collapse
          puts("collapse");

          // Covers annihilation , light-matter
          // and scattering.

          drf->k = COLLAPSE;
          lst->k = COLLAPSE;
          drf->obj = drf->a1;
          lst->obj = lst->a1;
          break;

        case 7:    // Internal collision
          puts("cohesion");

          // Cohesion.
          // (Sect. 4.1)

          if (stb->k == FERMION && nxt->k == FERMION)
          {
            CPNEG(drf->po, drf->p);
            CPNEG(lst->po, lst->p);
          }
          else if (stb->k == WB && nxt->k == WB)
          {
            CPNEG(lst->po, lst->p);
          }
          else if (stb->k == ZB && nxt->k == ZB)
          {
            CPNEG(lst->po, lst->p);
          }
          break;
      }
    }

    // Inter-sector interaction.
    // (see Sect. 4.7.6)

    else
    {
      // Super photon x fermion.

      if (nxt->k == FERMION && stb->k == SPHOTON)
      {
        CPNEG(drf->po, drf->p);
      }
      else if (stb->k == FERMION && nxt->k == SPHOTON)
      {
        CPNEG(lst->po, lst->p);
      }
    }
  }
}

#endif
