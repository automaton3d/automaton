/*
 * wires.c
 *
 *  Created on: 2 de mai. de 2023
 *      Author: Alexandre
 */
#include "simulation.h"

void wires(Cell *ptr, int x0, int y0, int z0, Cell *latt, int oE)
{
    // Wires

    if(x0 == 0)
    {
      if(y0 == 0)
      {
    	if(z0 == 0)
    	{
    	  ptr->ws[0] = latt + OFFSET(1, 0, 0) + oE;
    	  ptr->ws[1] = latt + OFFSET(SIDE - 1, 0, 0) + oE;
    	  ptr->ws[2] = latt + OFFSET(0, 1, 0) + oE;
    	  ptr->ws[3] = latt + OFFSET(0, SIDE - 1, 0) + oE;
    	  ptr->ws[4] = latt + OFFSET(0, 0, 1) + oE;
    	  ptr->ws[5] = latt + OFFSET(0, 0, SIDE - 1) + oE;
    	}
    	else if (z0 == SIDE - 1)
    	{
    	  ptr->ws[0] = latt + OFFSET(1, 0, z0) + oE;
          ptr->ws[1] = latt + OFFSET(SIDE - 1, 0, z0) + oE;
          ptr->ws[2] = latt + OFFSET(0, 1, z0) + oE;
          ptr->ws[3] = latt + OFFSET(0, SIDE - 1, z0) + oE;
          ptr->ws[4] = latt + OFFSET(0, 0, 0) + oE;
          ptr->ws[5] = latt + OFFSET(0, 0, z0 - 1) + oE;
	    }
	    else
	    {
          ptr->ws[0] = latt + OFFSET(1, 0, z0) + oE;
          ptr->ws[1] = latt + OFFSET(SIDE - 1, 0, z0) + oE;
          ptr->ws[2] = latt + OFFSET(0, 1, z0) + oE;
          ptr->ws[3] = latt + OFFSET(0, SIDE - 1, z0) + oE;
          ptr->ws[4] = latt + OFFSET(0, 0, z0 + 1) + oE;
          ptr->ws[5] = latt + OFFSET(0, 0, z0 - 1) + oE;
	    }
      }
      else if (y0 == SIDE - 1)
      {
	    if(z0 == 0)
	    {
          ptr->ws[0] = latt + OFFSET(1, y0, 0) + oE;
          ptr->ws[1] = latt + OFFSET(SIDE - 1, y0, 0) + oE;
          ptr->ws[2] = latt + OFFSET(0, 0, 0) + oE;
          ptr->ws[3] = latt + OFFSET(0, y0 - 1, 0) + oE;
          ptr->ws[4] = latt + OFFSET(0, y0, 1) + oE;
          ptr->ws[5] = latt + OFFSET(0, y0, SIDE - 1) + oE;
	    }
	    else if (z0 == SIDE - 1)
	    {
          ptr->ws[0] = latt + OFFSET(1, y0, z0) + oE;
          ptr->ws[1] = latt + OFFSET(SIDE - 1, y0, z0) + oE;
          ptr->ws[2] = latt + OFFSET(0, 0, z0) + oE;
          ptr->ws[3] = latt + OFFSET(0, y0 - 1, z0) + oE;
          ptr->ws[4] = latt + OFFSET(0, y0, 0) + oE;
          ptr->ws[5] = latt + OFFSET(0, y0, z0 - 1) + oE;
	    }
	    else
	    {
          ptr->ws[0] = latt + OFFSET(1, y0, z0) + oE;
          ptr->ws[1] = latt + OFFSET(SIDE - 1, y0, z0) + oE;
          ptr->ws[2] = latt + OFFSET(0, 0, z0) + oE;
          ptr->ws[3] = latt + OFFSET(0, y0 - 1, z0) + oE;
          ptr->ws[4] = latt + OFFSET(0, y0, z0 + 1) + oE;
          ptr->ws[5] = latt + OFFSET(0, y0, z0 - 1) + oE;
	    }
      }
      else
      {
    	if(z0 == 0)
    	{
          ptr->ws[0] = latt + OFFSET(1, y0, z0) + oE;
          ptr->ws[1] = latt + OFFSET(SIDE - 1, y0, z0) + oE;
          ptr->ws[2] = latt + OFFSET(0, y0 + 1, z0) + oE;
          ptr->ws[3] = latt + OFFSET(0, y0 - 1, z0) + oE;
          ptr->ws[4] = latt + OFFSET(0, y0, 1) + oE;
          ptr->ws[5] = latt + OFFSET(0, y0, SIDE - 1) + oE;
	    }
	    else if (z0 == SIDE - 1)
	    {
        	ptr->ws[0] = latt + OFFSET(1, y0, z0) + oE;
        	ptr->ws[1] = latt + OFFSET(SIDE - 1, y0, z0) + oE;
        	ptr->ws[2] = latt + OFFSET(0, y0 + 1, z0) + oE;
        	ptr->ws[3] = latt + OFFSET(0, y0 - 1, z0) + oE;
        	ptr->ws[4] = latt + OFFSET(0, y0, 0) + oE;
        	ptr->ws[5] = latt + OFFSET(0, y0, z0 - 1) + oE;
	    }
	    else
	    {
        	ptr->ws[0] = latt + OFFSET(1, y0, z0) + oE;
        	ptr->ws[1] = latt + OFFSET(SIDE - 1, y0, z0) + oE;
        	ptr->ws[2] = latt + OFFSET(0, y0 + 1, z0) + oE;
        	ptr->ws[3] = latt + OFFSET(0, y0 - 1, z0) + oE;
        	ptr->ws[4] = latt + OFFSET(0, y0, z0 + 1) + oE;
        	ptr->ws[5] = latt + OFFSET(0, y0, z0 - 1) + oE;
	    }
      }
    }
    else if (x0 == SIDE - 1)
    {
    	if(y0 == 0)
    	{
    		if(z0 == 0)
    		{
    	        	ptr->ws[0] = latt + OFFSET(0 , y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, 1, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, SIDE - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, 1) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, SIDE - 1) + oE;
    		}
    		else if (z0 == SIDE - 1)
    		{
    	        	ptr->ws[0] = latt + OFFSET(0, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, 1, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, SIDE - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, 0) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, z0 - 1) + oE;
    		}
    		else
    		{
    	        	ptr->ws[0] = latt + OFFSET(0, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, 1, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, SIDE - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, z0 + 1) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, z0 - 1) + oE;
    		}
    	}
    	else if (y0 == SIDE - 1)
    	{
    		if(z0 == 0)
    		{
    	        	ptr->ws[0] = latt + OFFSET(0, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, 0, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, y0 - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, 1) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, SIDE - 1) + oE;
    		}
    		else if (z0 == SIDE - 1)
    		{
    	        	ptr->ws[0] = latt + OFFSET(0, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, 0, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, y0 - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, 0) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, z0 - 1) + oE;
    		}
    		else
    		{
    	        	ptr->ws[0] = latt + OFFSET(0, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, y0 + 1, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, y0 - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, z0 + 1) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, z0 - 1) + oE;
    		}
    	}
    	else
    	{
    		if(z0 == 0)
    		{
    	        	ptr->ws[0] = latt + OFFSET(0, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, y0 + 1, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, y0 - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, 1) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, SIDE - 1) + oE;
    		}
    		else if (z0 == SIDE - 1)
    		{
    	        	ptr->ws[0] = latt + OFFSET(0, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, y0 + 1, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, y0 - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, 0) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, z0 - 1) + oE;
    		}
    		else
    		{
    	        	ptr->ws[0] = latt + OFFSET(0, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, y0 + 1, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, y0 - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, z0 + 1) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, z0 - 1) + oE;
    		}
    	}
    }
    else
    {
    	if(y0 == 0)
    	{
    		if(z0 == 0)
    		{
    	        	ptr->ws[0] = latt + OFFSET(x0 + 1, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, 1, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, SIDE - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, 1) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, SIDE - 1) + oE;
    		}
    		else if (z0 == SIDE - 1)
    		{
    	        	ptr->ws[0] = latt + OFFSET(x0 + 1, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, 1, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, SIDE - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, 0) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, z0 - 1) + oE;
    		}
    		else
    		{
    	        	ptr->ws[0] = latt + OFFSET(x0 + 1, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, 1, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, SIDE - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, z0 + 1) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, z0 - 1) + oE;
    		}
    	}
    	else if (y0 == SIDE - 1)
    	{
    		if(z0 == 0)
    		{
    	        	ptr->ws[0] = latt + OFFSET(x0 + 1, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, 0, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, y0 - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, 1) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, SIDE - 1) + oE;
    		}
    		else if (z0 == SIDE - 1)
    		{
    	        	ptr->ws[0] = latt + OFFSET(x0 + 1, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, 0, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, y0 - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, 0) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, z0 - 1) + oE;
    		}
    		else
    		{
    	        	ptr->ws[0] = latt + OFFSET(x0 + 1, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, 0, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, y0 - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, z0 + 1) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, z0 - 1) + oE;
    		}
    	}
    	else
    	{
    		if(z0 == 0)
    		{
    	        	ptr->ws[0] = latt + OFFSET(x0 + 1, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, y0 + 1, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, y0 - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, 1) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, SIDE - 1) + oE;
    		}
    		else if (z0 == SIDE - 1)
    		{
    	        	ptr->ws[0] = latt + OFFSET(x0 + 1, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, y0 + 1, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, y0 - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, 0) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, z0 - 1) + oE;
    		}
    		else
    		{
    	        	ptr->ws[0] = latt + OFFSET(x0 + 1, y0, z0) + oE;
    	        	ptr->ws[1] = latt + OFFSET(x0 - 1, y0, z0) + oE;
    	        	ptr->ws[2] = latt + OFFSET(x0, y0 + 1, z0) + oE;
    	        	ptr->ws[3] = latt + OFFSET(x0, y0 - 1, z0) + oE;
    	        	ptr->ws[4] = latt + OFFSET(x0, y0, z0 + 1) + oE;
    	        	ptr->ws[5] = latt + OFFSET(x0, y0, z0 - 1) + oE;
    		}
    	}
    }
}
