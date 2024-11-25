/*
 * stuff.cpp
 *
 * Auxiliary routines used by the simulation.
 */

#include "simulation.h"

namespace automaton
{
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

	/*
	 * Executes the relocation of one cell.
	 */
	void relocate(Cell *draft, Cell *nei)
	{
		COPY(draft->pos, nei->pos);
		COPY(draft->c, nei->c);
		draft->d = nei->d;
		draft->sin = nei->sin;
		draft->aff = nei->aff;
		draft->e = nei->e;
	}

	/*
	 * Mark the poles of a given momentum
	 * Uses the Bresenham line algorithm.
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

	    dx = std::abs(dx);
	    dy = std::abs(dy);
	    dz = std::abs(dz);

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

}
