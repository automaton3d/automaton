#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "combine.h"

int frags[N];
int control[N];

// Total

long t_e_O, t_e_D;
long t_e_O_bar, t_e_D_bar;
long t_q_O, t_q_D;
long t_q_O_bar, t_q_D_bar;
long t_v_O, t_v_D;
long t_nu_O;
long t_gl_O;
long t_up_O;
long t_up_O_bar;
long t_ph_O;
long t_PH_O;
long t_zb_O;
long t_wb_O;
long t_nu_D;
long t_gl_D;
long t_up_D;
long t_up_D_bar;
long t_ph_D;
long t_PH_D;
long t_zb_D;
long t_wb_D;

// Partial

int n_e = 0;
int n_e_D = 0;
int n_e_bar = 0;
int n_e_D_bar = 0;
int n_q = 0;
int n_q_D = 0;
int n_q_bar = 0;
int n_q_D_bar = 0;
int n_v = 0;
int n_v_D = 0;
int n_nu = 0;
int n_nu_D = 0;
int n_gl = 0;
int n_gl_D = 0;
int n_up = 0;
int n_up_bar = 0;
int n_up_D = 0;
int n_up_D_bar = 0;
int n_ph = 0;
int n_ph_D = 0;
int n_PH = 0;
int n_PH_D = 0;
int n_zb = 0;
int n_zb_D = 0;
int n_wb = 0;
int n_wb_D = 0;

int rgb0 = 1;
int rgb1 = 1;

int last = 0;

int tot_O, tot_D;

//extern int total;

//extern int matter, antimatter;

int anti(int frag)
{
	return ~frag & 0x3f;
}

int tryGluon()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag0 = frags[i];
		int c0   = frag0 & C_MASK;
		int w0_0 = (frag0 & W0_MASK)>>3;
		int w1_0 = (frag0 & W1_MASK)>>4;
		int q0   = (frag0 & Q_MASK)>>5;
		int m0   = (c0 == 1 || c0 == 2 || c0 == 4);
		if(w1_0 == 1 || w0_0 == 0 || c0 == 0 || c0 ==7)
			continue;
		for(int j = 0; j < N; j++)
		{
			int frag1 = frags[j];
			if(i == j || control[j])
				continue;
			int c1   = frag1 & C_MASK;
			int w0_1 = (frag1 & W0_MASK)>>3;
			int w1_1 = (frag1 & W1_MASK)>>4;
			int q1   = (frag1 & Q_MASK)>>5;
			int m1   = (c1 == 1 || c1 == 2 || c1 == 4);
			//
			// Orbis
			//
			if(w1_1 == 1 || w0_1 == 0 || m0 == m1 || q0 == q1 || c0 == c1 || c1 == 0 || c1 == 7)
				continue;
			n_gl += 2;
			control[i] = 1;
			control[j] = 1;
			return 1;
		}
	}
	return 0;
}

int tryGluon_D()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag0 = frags[i];
		int c0   = frag0 & C_MASK;
		int w0_0 = (frag0 & W0_MASK)>>3;
		int w1_0 = (frag0 & W1_MASK)>>4;
		int q0   = (frag0 & Q_MASK)>>5;
		int m0   = (c0 == 1 || c0 == 2 || c0 == 4);
		if(w1_0 == 0 || w0_0 == 1 || c0 == 0 || c0 ==7)
			continue;
		for(int j = 0; j < N; j++)
		{
			int frag1 = frags[j];
			if(i == j || control[j])
				continue;
			int c1   = frag1 & C_MASK;
			int w0_1 = (frag1 & W0_MASK)>>3;
			int w1_1 = (frag1 & W1_MASK)>>4;
			int q1   = (frag1 & Q_MASK)>>5;
			int m1   = (c1 == 1 || c1 == 2 || c1 == 4);
			//
			// Dark
			//
			if(w1_1 == 0 || w0_1 == 1 || m0 == m1 || q0 == q1 || c0 == c1 || c1 == 0 || c1 == 7)
				continue;
			n_gl_D += 2;
			control[i] = 1;
			control[j] = 1;
			return 1;
		}
	}
	return 0;
}

int tryUp()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag = frags[i];
		int c  = frag & C_MASK;
		int w0 = (frag & W0_MASK)>>3;
		int w1 = (frag & W1_MASK)>>4;
		int q  = (frag & Q_MASK)>>5;
		int m  = (c == 1 || c == 2 || c == 4);
		if(w1 == 1 || w0 == 0 || q == 1 || c == 0 || c == 7 || !m)
			continue;
		for(int j = 0; j < N; j++)
		{
			int frag1 = frags[j];
			if(i == j || control[j] || frag != frag1)
				continue;
			n_up += 2;
			control[i] = 1;
			control[j] = 1;
			//
			while(n_q < n_up / 4)
				tryQuark();
			while(n_e < n_up * 3 / 4)
				tryElec();
			//
			return 1;
		}
	}
	return 0;
}

int tryUp_bar()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag = frags[i];
		int c  = frag & C_MASK;
		int w0 = (frag & W0_MASK)>>3;
		int w1 = (frag & W1_MASK)>>4;
		int q  = (frag & Q_MASK)>>5;
		int m  = (c == 1 || c == 2 || c == 4);
		if(w1 == 1 || w0 == 1 || q == 0 || c == 0 || c == 7 || !m)
			continue;
		for(int j = 0; j < N; j++)
		{
			int frag1 = frags[j];
			if(i == j || control[j] || frag != frag1)
				continue;
			n_up_bar += 2;
			control[i] = 1;
			control[j] = 1;
			//
//			while(n_q < n_up / 4)
//				tryQuark();
//			while(n_e < n_up * 3 / 4)
//				tryElec();
			//
			return 1;
		}
	}
	return 0;
}

int tryUp_D()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag = frags[i];
		int c  = frag & C_MASK;
		int w0 = (frag & W0_MASK)>>3;
		int w1 = (frag & W1_MASK)>>4;
		int q  = (frag & Q_MASK)>>5;
		int m  = (c == 1 || c == 2 || c == 4);
		if(w1 == 0 || w0 == 1 || q == 0 || c == 0 || c == 7 || m)
			continue;
		for(int j = 0; j < N; j++)
		{
			int frag1 = frags[j];
			if(i == j || control[j] || frag != frag1)
				continue;
			n_up_D += 2;
			control[i] = 1;
			control[j] = 1;
			while(n_q_D < n_up_D / 4)
				tryQuark_D();
			while(n_e_D < n_up_D * 3 / 4)
				tryElec_D();
			return 1;
		}
	}
	return 0;
}

int tryUp_D_bar()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag = frags[i];
		int c  = frag & C_MASK;
		int w0 = (frag & W0_MASK)>>3;
		int w1 = (frag & W1_MASK)>>4;
		int q  = (frag & Q_MASK)>>5;
		int m  = (c == 1 || c == 2 || c == 4);
		if(w1 == 0 || w0 == 0 || q == 1 || c == 0 || c == 7 || m)
			continue;
		for(int j = 0; j < N; j++)
		{
			int frag1 = frags[j];
			if(i == j || control[j] || frag != frag1)
				continue;
			n_up_D_bar += 2;
			control[i] = 1;
			control[j] = 1;
//			while(n_q_D < n_up_D / 4)
//				tryQuark_D();
//			while(n_e_D < n_up_D * 3 / 4)
//				tryElec_D();
			return 1;
		}
	}
	return 0;
}

int tryQuark()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag = frags[i];
		for(int k = 0; k < 3; k++)
		{
			rgb0 <<= 1;
			if(rgb0 == 4)
				rgb0 = 1;
			if(frag == (0x28 | rgb0))
			{
				control[i] = 1;
				n_q++;
				return 1;
			}
		}
	}
	return 0;
}

int tryQuark_bar()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag = frags[i];
		for(int k = 0; k < 3; k++)
		{
			rgb0 <<= 1;
			if(rgb0 == 4)
				rgb0 = 1;
			if(frag == (0x07 | rgb0))
			{
				control[i] = 1;
				n_q_bar++;
				return 1;
			}
		}
	}
	return 0;
}

int tryQuark_D()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag = frags[i];
		for(int k = 0; k < 3; k++)
		{
			rgb0 <<= 1;
			if(rgb0 == 4)
			rgb0 = 1;
			if(frag == anti(0x28 | rgb0))
			{
				control[i] = 1;
				n_q_D++;
				return 1;
			}
		}
	}
	return 0;
}

int tryQuark_D_bar()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag = frags[i];
		for(int k = 0; k < 3; k++)
		{
			rgb0 <<= 1;
			if(rgb0 == 4)
			rgb0 = 1;
			if(frag == anti(0x07 | rgb0))
			{
				control[i] = 1;
				n_q_D_bar++;
				return 1;
			}
		}
	}
	return 0;
}

int tryElec()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag = frags[i];
		if(frag == 0x28)
		{
			control[i] = 1;
			n_e++;
			return 1;
		}
	}
	return 0;
}

int tryElec_bar()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag = frags[i];
		if(frag == 0x07)
		{
			control[i] = 1;
			n_e_bar++;
			return 1;
		}
	}
	return 0;
}

int tryElec_D()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag = frags[i];
		if(frag == anti(0x28))
		{
			control[i] = 1;
			n_e_D++;
			return 1;
		}
	}
	return 0;
}

int tryElec_D_bar()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag = frags[i];
		if(frag == anti(0x07))
		{
			control[i] = 1;
			n_e_D_bar++;
			return 1;
		}
	}
	return 0;
}

int tryVirtual()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag = frags[i];
		for(int k = 0; k < 3; k++)
		{
			rgb0 <<= 1;
			if(rgb0 == 4)
				rgb0 = 1;
			if(frag == (0x28 | rgb0))
			{
				control[i] = 1;
				n_v++;
				return 1;
			}
		}
	}
	return 0;
}

int tryVirtual_D()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag = frags[i];
		for(int k = 0; k < 3; k++)
		{
			rgb0 <<= 1;
			if(rgb0 == 4)
			rgb0 = 1;
			if(frag == anti(0x28 | rgb0))
			{
				control[i] = 1;
				n_v_D++;
				return 1;
			}
		}
	}
	return 0;
}

int tryNeutrino()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag0 = frags[i];
		if(frag0 == 0x08 || frag0 == 0x28)
		{
			for(int j = 0; j < N; j++)
			{
				if(i == j || control[j])
					continue;
				int frag1 = frags[j];
				if(frag0 != frag1)
				{
					if(frag1 == 0x28 || frag1 == 0x08)
					{
						control[i] = 1;
						control[j] = 1;
						n_nu += 2;
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

int tryNeutrino_D()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag0 = frags[i];
		if(frag0 == anti(0x08) || frag0 == anti(0x28))
		{
			for(int j = 0; j < N; j++)
			{
				if(i == j || control[j])
					continue;
				int frag1 = frags[j];
				if(frag0 != frag1)
				{
					if(frag1 == anti(0x08) || frag1 == anti(0x28))
					{
						control[i] = 1;
						control[j] = 1;
						n_nu_D += 2;
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

int tryPhoton()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag0 = frags[i];
		if(frag0 == 0x08 || frag0 == 0x28)
		{
			for(int j = 0; j < N; j++)
			{
				if(i == j || control[j])
					continue;
				int frag1 = frags[j];
				int c0   = frag0 & C_MASK;
				int c1   = frag1 & C_MASK;
				int w0_0 = (frag0 & W0_MASK)>>3;
				int w0_1 = (frag1 & W0_MASK)>>3;
				int w1_0 = (frag0 & W1_MASK)>>4;
				int w1_1 = (frag1 & W1_MASK)>>4;
				int q0   = (frag0 & Q_MASK)>>5;
				int q1   = (frag1 & Q_MASK)>>5;
				//
				if(w1_0 != w1_1 || q0 == q1 || w0_0 == w0_1 || ((c0 ^ c1) & 7) != 7)
					continue;
				//
				n_ph += 2;
				control[i] = 1;
				control[j] = 1;
				return 1;
			}
		}
	}
	return 0;
}

int tryPhoton_D()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag0 = frags[i];
		if(frag0 == anti(0x08) || frag0 == anti(0x28))
		{
			for(int j = 0; j < N; j++)
			{
				if(i == j || control[j])
					continue;
				int frag1 = frags[j];
				int c0   = frag0 & C_MASK;
				int c1   = frag1 & C_MASK;
				int w0_0 = (frag0 & W0_MASK)>>3;
				int w0_1 = (frag1 & W0_MASK)>>3;
				int w1_0 = (frag0 & W1_MASK)>>4;
				int w1_1 = (frag1 & W1_MASK)>>4;
				int q0   = (frag0 & Q_MASK)>>5;
				int q1   = (frag1 & Q_MASK)>>5;
				//
				if(w1_0 != w1_1 || q0 == q1 || w0_0 == w0_1 || ((c0 ^ c1) & 7) != 7)
					continue;
				//
				n_ph_D += 2;
				control[i] = 1;
				control[j] = 1;
				return 1;
			}
		}
	}
	return 0;
}

int tryPHOTON()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag0 = frags[i];
		if(frag0 == 0x08 || frag0 == 0x28)
		{
			for(int j = 0; j < N; j++)
			{
				if(i == j || control[j])
					continue;
				int frag1 = frags[j];
				int c0   = frag0 & C_MASK;
				int c1   = frag1 & C_MASK;
				int w0_0 = (frag0 & W0_MASK)>>3;
				int w0_1 = (frag1 & W0_MASK)>>3;
				int w1_0 = (frag0 & W1_MASK)>>4;
				int w1_1 = (frag1 & W1_MASK)>>4;
				int q0   = (frag0 & Q_MASK)>>5;
				int q1   = (frag1 & Q_MASK)>>5;
				//
				if(w1_0 == w1_1 || q0 == q1 || w0_0 == w0_1 || ((c0 ^ c1) & 7) != 7)
					continue;
				//
				n_PH += 2;
				control[i] = 1;
				control[j] = 1;
				return 1;
			}
		}
	}
	return 0;
}

int tryPHOTON_D()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag0 = frags[i];
		if(frag0 == anti(0x08) || frag0 == anti(0x28))
		{
			for(int j = 0; j < N; j++)
			{
				if(i == j || control[j])
					continue;
				int frag1 = frags[j];
				int c0   = frag0 & C_MASK;
				int c1   = frag1 & C_MASK;
				int w0_0 = (frag0 & W0_MASK)>>3;
				int w0_1 = (frag1 & W0_MASK)>>3;
				int w1_0 = (frag0 & W1_MASK)>>4;
				int w1_1 = (frag1 & W1_MASK)>>4;
				int q0   = (frag0 & Q_MASK)>>5;
				int q1   = (frag1 & Q_MASK)>>5;
				//
				if(w1_0 == w1_1 || q0 == q1 || w0_0 == w0_1 || ((c0 ^ c1) & 7) != 7)
					continue;
				//
				n_PH_D += 2;
				control[i] = 1;
				control[j] = 1;
				return 1;
			}
		}
	}
	return 0;
}

int tryWB()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag0 = frags[i];
		int c0   = frag0 & C_MASK;
		int w0_0 = (frag0 & W0_MASK)>>3;
		int w1_0 = (frag0 & W1_MASK)>>4;
		int q0   = (frag0 & Q_MASK)>>5;
		if(w0_0 == 0 || w1_0 == 1)
			return 0;
		for(int j = 0; j < N; j++)
		{
			if(i == j || control[j])
				continue;
			int frag1 = frags[j];
			int c1   = frag1 & C_MASK;
//			int w0_1 = (frag1 & W0_MASK)>>3;
			int w1_1 = (frag1 & W1_MASK)>>4;
			int q1   = (frag1 & Q_MASK)>>5;
			//
			if(w1_1 == 1 || q0 != q1 || /* (w0_0 | w0_1) == 0 || */ ((c0 ^ c1) & 7) != 7)
				continue;
			//
			n_wb += 2;
			control[i] = 1;
			control[j] = 1;
			return 1;
		}
	}
	return 0;
}

int tryWB_D()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag0 = frags[i];
		int c0   = frag0 & C_MASK;
//		int w0_0 = (frag0 & W0_MASK)>>3;
		int w1_0 = (frag0 & W1_MASK)>>4;
		int q0   = (frag0 & Q_MASK)>>5;
		if(w1_0 == 0)
			return 0;
		for(int j = 0; j < N; j++)
		{
			if(i == j || control[j])
				continue;
			int frag1 = frags[j];
			int c1   = frag1 & C_MASK;
//			int w0_1 = (frag1 & W0_MASK)>>3;
			int w1_1 = (frag1 & W1_MASK)>>4;
			int q1   = (frag1 & Q_MASK)>>5;
			//
			if(w1_1 == 0 || q0 != q1 || /*(w0_0 & w0_1) == 1 ||*/ ((c0 ^ c1) & 7) != 7)
				continue;
			//
			n_wb_D += 2;
			control[i] = 1;
			control[j] = 1;
			return 1;
		}
	}
	return 0;
}

int tryZB()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag0 = frags[i];
		int c0   = frag0 & C_MASK;
//		int w0_0 = (frag0 & W0_MASK)>>3;
		int w1_0 = (frag0 & W1_MASK)>>4;
		int q0   = (frag0 & Q_MASK)>>5;
		if(w1_0 == 1)
			return 0;
		for(int j = 0; j < N; j++)
		{
			if(i == j || control[j])
				continue;
			int frag1 = frags[j];
			int c1   = frag1 & C_MASK;
//			int w0_1 = (frag1 & W0_MASK)>>3;
			int w1_1 = (frag1 & W1_MASK)>>4;
			int q1   = (frag1 & Q_MASK)>>5;
			//
			if(w1_1 == 1 || q0 == q1 || /*(w0_0 | w0_1) == 0 ||*/ ((c0 ^ c1) & 7) != 7)
				continue;
			//
			n_zb += 2;
			control[i] = 1;
			control[j] = 1;
			return 1;
		}
	}
	return 0;
}

int tryZB_D()
{
	for(int i = 0; i < N; i++)
	{
		if(control[i])
			continue;
		int frag0 = frags[i];
		int c0   = frag0 & C_MASK;
//		int w0_0 = (frag0 & W0_MASK)>>3;
		int w1_0 = (frag0 & W1_MASK)>>4;
		int q0   = (frag0 & Q_MASK)>>5;
		if(w1_0 == 0)
			return 0;
		for(int j = 0; j < N; j++)
		{
			if(i == j || control[j])
				continue;
			int frag1 = frags[j];
			int c1   = frag1 & C_MASK;
//			int w0_1 = (frag1 & W0_MASK)>>3;
			int w1_1 = (frag1 & W1_MASK)>>4;
			int q1   = (frag1 & Q_MASK)>>5;
			//
			if(w1_1 == 0 || q0 == q1 /*|| (w0_0 & w0_1) == 1*/ || ((c0 ^ c1) & 7) != 7)
				continue;
			//
			n_zb_D += 2;
			control[i] = 1;
			control[j] = 1;
			return 1;
		}
	}
	return 0;
}

void combine()
{
	for(int i = 0; i < N; i++)
	{
		frags[i] = i & 0x3f;
		control[i] = 0;
	}
	//
	n_e = 0;
	n_e_D = 0;
	n_e_bar = 0;
	n_e_D_bar = 0;
	n_q = 0;
	n_q_D = 0;
	n_q_bar = 0;
	n_q_D_bar = 0;
	n_v = 0;
	n_v_D = 0;
	n_nu = 0;
	n_nu_D = 0;
	n_gl = 0;
	n_gl_D = 0;
	n_up = 0;
	n_up_D = 0;
	n_up_bar = 0;
	n_up_D_bar = 0;
	n_ph = 0;
	n_ph_D = 0;
	n_PH = 0;
	n_PH_D = 0;
	n_zb = 0;
	n_zb_D = 0;
	n_wb = 0;
	n_wb_D = 0;
	//
	putchar('.');
	for(int i = 0; i < N; i++)
	{
		double prob = rand() / (double)RAND_MAX;
		if(prob < 0.001)
		{
			tryUp();
			tryUp_D();
		}
		else
		{
			tryGluon();
			tryGluon_D();
		}
	}
	putchar('.');
	for(int i = 0; i < N; i++)
	{
		tryVirtual();
		tryVirtual_D();
	}
	putchar('.');
	for(int i = 0; i < N; i++)
	{
		tryNeutrino();
		tryNeutrino_D();
	}
	putchar('.');
	for(int i = 0; i < N; i++)
	{
		tryPhoton();
		tryPHOTON();
		tryPhoton_D();
		tryPHOTON_D();
	}
	putchar('.');
	for(int i = 0; i < N; i++)
	{
		tryZB();
		tryWB();
		tryZB_D();
		tryWB_D();
	}
	putchar('.');
	tryUp_bar();
	putchar('.');
	tryUp_D_bar();
	putchar('.');
	putchar('.');
	tryElec_bar();
	putchar('.');
	tryElec_D_bar();
	t_e_O += n_e;
	t_e_O_bar += n_e_bar;
	t_q_O += n_q;
	t_q_O_bar += n_q_bar;
	t_v_O += n_v;
	t_nu_O += n_nu;
	t_gl_O += n_gl;
	t_up_O += n_up;
	t_up_O_bar += n_up_bar;
	t_ph_O += n_ph;
	t_PH_O += n_PH;
	t_zb_O += n_zb;
	t_wb_O += n_wb;
	t_e_D += n_e_D;
	t_q_D += n_q_D;
	t_v_D += n_v_D;
	t_nu_D += n_nu_D;
	t_gl_D += n_gl_D;
	t_up_D += n_up_D;
	t_up_D_bar += n_up_D_bar;
	t_ph_D += n_ph_D;
	t_PH_D += n_PH_D;
	t_zb_D += n_zb_D;
	t_wb_D += n_wb_D;
}

void leftover()
{
	for(int i = 0; i < N; i++)
	{
		if(!control[i])
		{
			int frag = frags[i];
			if((frag & W1_MASK)>>4)
				tot_O++;
			else
				tot_D++;
		}
	}
}

int optimize()
{
	t_e_O = 0;
	t_e_D = 0;
	t_e_O_bar = 0;
	t_e_D_bar = 0;
	t_q_O = 0;
	t_q_D = 0;
	t_v_O = 0;
	t_v_D = 0;
	t_nu_O = 0;
	t_nu_D = 0;
	t_gl_O = 0;
	t_gl_D = 0;
	t_up_O = 0;
	t_up_D = 0;
	t_ph_O = 0;
	t_ph_D = 0;
	t_PH_O = 0;
	t_PH_D = 0;
	t_zb_O = 0;
	t_zb_D = 0;
	t_wb_O = 0;
	t_wb_D = 0;
	tot_O = 0;
	tot_D = 0;
	printf("\n-- ZOOOOO --\n\nN=%d NR=%d\n\n", N, (int)REPEAT);
	for(int i = 0; i < REPEAT; i++)
	{
		combine();
		leftover();
		putchar('.');
	}
	printf("\n\nFragment\tType\t\tQuantity\n");
	printf("----------------------------------------------------------------------\n");
	printf("gluon\t\tPair\t\t%.1f\t%.1f\n", t_gl_O / REPEAT, t_gl_D / REPEAT);
	printf("up quark\tPair\t\t%.1f\t%.1f\n", t_up_O / REPEAT, t_up_D / REPEAT);
	printf("quark\t\tSingle\t\t%.1f\t%.1f\n", t_q_O / REPEAT, t_q_D / REPEAT);
	printf("virtual\t\tSingle\t\t%.1f\t%.1f\n", t_v_O / REPEAT, t_v_D / REPEAT);
	printf("elec\t\tSingle\t\t%.1f\t%.1f\n", t_e_O / REPEAT, t_e_D / REPEAT);
	printf("photon\t\tPair\t\t%.1f\t%.1f\n", t_ph_O / REPEAT, t_ph_D / REPEAT);
	printf("PHoton\t\tPair\t\t%.1f\t%.1f\n", t_PH_O / REPEAT, t_PH_D / REPEAT);
	printf("Z boson\t\tPair\t\t%.1f\t%.1f\n", t_zb_O / REPEAT, t_zb_D / REPEAT);
	printf("W boson\t\tPair\t\t%.1f\t%.1f\n", t_wb_O / REPEAT, t_wb_D / REPEAT);
	printf("neutrino\tPair\t\t%.1f\t%.1f\n", t_nu_O / REPEAT, t_nu_D / REPEAT);
	printf("up_bar\t\tPair\t\t%.1f\t%.1f\n", t_up_O_bar / REPEAT, t_up_D_bar / REPEAT);
	printf("elec_bar\tSingle\t\t%.1f\t%.1f\n", t_e_O_bar / REPEAT, t_e_D_bar / REPEAT);
	printf("q_bar\t\tSingle\t\t%.1f\t%.1f\n", t_q_O_bar / REPEAT, t_q_D_bar / REPEAT);
	printf("leftover\tSingle\t\t%.1f\t%.1f\t\t\n", tot_O / REPEAT, tot_D / REPEAT);
	int totO = t_nu_O + t_gl_O + t_up_O + t_ph_O + t_PH_O + t_zb_O + t_wb_O + t_e_O + t_q_O + t_v_O;
	int totD = t_nu_D + t_gl_D + t_up_D + t_ph_D + t_PH_D + t_zb_D + t_wb_D + t_e_D + t_q_D + t_v_D;
	printf("Total\t\t\t\t%.1f\t(perdido: %0.1f%%)\n", (totO + totD + tot_O + tot_D) / REPEAT, 100*(tot_O + tot_D) / (double)N);
	printf("----------------------------------------------------------------------\n");
	float ratioO = (t_gl_O + t_up_O + t_q_O) / (float)t_e_O;
	float ratioD = (t_gl_D + t_up_D + t_q_D) / (float)t_e_D;
	printf("m_p/m_e: O=%f\t\tD=%f\n", ratioO, ratioD);
	return 0;
}

int main (void)
{
	setvbuf(stdout, NULL, _IONBF, 0);
#define OPTIMIZE
#ifdef OPTIMIZE
	//srand(time(0));
	optimize();
#endif
	////////////////
//#define REVERSE
#ifdef REVERSE
	orbis();
	init();
	dark();
	printf("\nTotal=%d : %d\n", total, matter + antimatter);
#endif
	for(int i = 0; i < 5; i++)
	{
		Beep(800, 200);
		Sleep(100);
	}
	return 0;
}
