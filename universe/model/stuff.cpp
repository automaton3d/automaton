/*
 * stuff.cpp
 *
 * Auxiliary routines used by the simulation.
 */

#include "simulation.h"

namespace automaton
{
	using namespace std;

	/*
	 * Counts the charges in dimension W.
	 */
	void compileNetCharges(Cell *mirror, Cell *draft)
	{
	    // Compile blindly net charges
	    if (mirror->charge & C0_MASK)
	      draft->net_c0++;
	    else
	      draft->net_c0--;
	    //
	    if (mirror->charge & C1_MASK)
	      draft->net_c1++;
	    else
	      draft->net_c1--;
	    //
	    if (mirror->charge & C2_MASK)
	      draft->net_c2++;
	    else
	      draft->net_c2--;
	    //
	    if (mirror->charge & Q_MASK)
	      draft->net_q++;
	    else
	      draft->net_q--;
	    //
	    if (mirror->charge & W0_MASK)
	      draft->net_w0++;
	    else
	      draft->net_w0--;
	    //
	    if (mirror->charge & W1_MASK)
	      draft->net_w1++;
	    else
	      draft->net_w1--;
	}

	void relocate(Cell& draft, const Cell& nei)
	{
	    // Copy the pos and c arrays element by element
	    copy(nei.pos, nei.pos + 3, draft.pos);  // Copy pos array
	    copy(nei.c, nei.c + 3, draft.c);        // Copy c array

	    // Copy other members
	    draft.d = nei.d;
	    draft.sin = nei.sin;
	    draft.aff = nei.aff;
	    draft.e = nei.e;
	}

	/**
	 * Marks the poles of a given momentum.
	 * Uses the Bresenham line algorithm.
	 * (used during initialization)
	 *
	 * @p the current ray
	 * @w the shell in the w dimension
	 */
	void markPoles(unsigned p[3], int w)
	{
	    // Calculate the center of the lattice
	    int cx = CENTER, cy = CENTER, cz = CENTER;
	    // Calculate the deltas
	    int dx = p[0] - cx;
	    int dy = p[1] - cy;
	    int dz = p[2] - cz;
	    // Determine the step direction
	    int sx = (dx > 0) ? 1 : -1;
	    int sy = (dy > 0) ? 1 : -1;
	    int sz = (dz > 0) ? 1 : -1;
	    dx = abs(dx);
	    dy = abs(dy);
	    dz = abs(dz);
	    // Determine the dominant axis
	    int ax = 2 * dx;
	    int ay = 2 * dy;
	    int az = 2 * dz;
	    int x = cx, y = cy, z = cz;
	    if (dx >= dy && dx >= dz)
	    {
	    	// X is dominant
	        int yd = ay - dx;
	        int zd = az - dx;
	        for (int i = 0; i <= dx; i++)
	        {
//	        	(lattice_current + w * SIDE3 + x * SIDE2 + y * SIDE + z)->pole = true;
	        	lattice_current[w * SIDE3 + x * SIDE2 + y * SIDE + z].pole = true;

	            x += sx;
	            if (yd >= 0)
	            {
	                y += sy;
	                yd -= ax;
	            }
	            if (zd >= 0)
	            {
	                z += sz;
	                zd -= ax;
	            }
	            yd += ay;
	            zd += az;
	        }
	    }
	    else if (dy >= dx && dy >= dz)
	    {
	    	// Y is dominant
	        int xd = ax - dy;
	        int zd = az - dy;
	        for (int i = 0; i <= dy; i++)
	        {
//	        	(lattice_current + w * SIDE3 + x * SIDE2 + y * SIDE + z)->pole = true;
	        	lattice_current[w * SIDE3 + x * SIDE2 + y * SIDE + z].pole = true;
	            y += sy;
	            if (xd >= 0)
	            {
	                x += sx;
	                xd -= ay;
	            }
	            if (zd >= 0)
	            {
	                z += sz;
	                zd -= ay;
	            }
	            xd += ax;
	            zd += az;
	        }
	    }
	    else
	    {
	    	// Z is dominant
	        int xd = ax - dz;
	        int yd = ay - dz;
	        for (int i = 0; i <= dz; i++)
	        {
//	        	(lattice_current + w * SIDE3 + x * SIDE2 + y * SIDE + z)->pole = true;
	        	lattice_current[w * SIDE3 + x * SIDE2 + y * SIDE + z].pole = true;
	            z += sz;
	            if (xd >= 0)
	            {
	                x += sx;
	                xd -= az;
	            }
	            if (yd >= 0)
	            {
	                y += sy;
	                yd -= az;
	            }
	            xd += ax;
	            yd += ay;
	        }
	    }
	}

	/*
	 * State function that selects specific properties of interest to
	 * represent the state of a cell.
	 */
	uint32_t cellState(unsigned x, unsigned y, unsigned z, Cell *cell)
	{
        return cell->charge
	               | (cell->aff << 6)
	               | (x << (6 + (ORDER - 1)))
	               | (y << (6 + (ORDER - 1) + ORDER))
	               | (z << (6 + (ORDER - 1) + 2 * ORDER));
	}

}
