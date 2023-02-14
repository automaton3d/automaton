/*
 ============================================================================
 Name        : combine.c
 Author      : Alexandre
 Version     :
 Copyright   : Your copyright notice
 Description : Charge combination
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N         (6 * 128)
#define ATTEMPTS  2000000
#define C_MASK    0x07
#define W0_MASK   0x08
#define W1_MASK   0x10
#define Q_MASK    0x20

char *binary[] =
{
	"000 N",
	"001 R",
	"010 G",
	"011 b",
	"100 B",
	"101 g",
	"110 r",
	"111 n"
};

char *color[] = { "N", "R", "G", "b", "B", "g", "r", "n" };
char *weak[] = { "R", "L" };
char *electric[] = { "+", "-" };
char *sector[] = { "O", "D" };

int sets[2][N];
int control[2][N];

int show = 0;

long t_e = 0;
long t_q = 0;

long t_nu = 0;
long t_gl = 0;
long t_up = 0;
long t_ph = 0;
long t_zb = 0;
long t_wb = 0;

int tpO = 0, tnO = 0, tpD = 0, tnD = 0;

long t_lo = 0;

int quarks[] =
{
	0x29, 0x2a, 0x2c, 0x06, 0x05, 0x03, 0x16, 0x15, 0x13, 0x39, 0x3a, 0x3c
};

int electrons[] =
{
	0x28, 0x07, 0x17, 0x38
};

char *formatFrag(int frag)
{
	static char s[20];
	sprintf(s, "[%s][%s%s][%s] ", electric[1 & (frag >> 5)], sector[1 & (frag>>4)], weak[1 & (frag>>3)], color[frag & C_MASK]);
	return s;
}

int tryElectron(int i, int j)
{
	if(control[j][i] == 0)
	{
		for(int k = 0; k < 4; k++)
		{
			if(sets[j][i] == electrons[k])
			{
				control[j][i] = 1;
				return 1;
			}
		}
	}
	return 0;
}

int tryQuark(int i, int j)
{
	if(control[j][i] == 0)
	{
		for(int k = 0; k < 12; k++)
		{
			if(sets[j][i] == quarks[k])
			{
				control[j][i] = 1;
				return 1;
			}
		}
	}
	return 0;
}

int tryWB(int i, int j)
{
	if(control[0][i] || control[1][j])
		return 0;
	int frag0 = sets[0][i];
	int frag1 = sets[1][j];
	//
	int c0   = frag0 & C_MASK;
	int c1   = frag1 & C_MASK;
	int w0_0 = frag0 & W0_MASK;
	int w0_1 = frag1 & W0_MASK;
	int w1_0 = frag0 & W1_MASK;
	int w1_1 = frag1 & W1_MASK;
	int q0   = frag0 & Q_MASK;
	int q1   = frag1 & Q_MASK;
	//
	if(q0 != q1 || w1_0 != w1_1 || w0_0 != w0_1)
		return 0;
	if(q0 != w0_0)
		return 0;
	if((c0 == 0 && c1 == 7) || (c0 == 7 && c1 == 0))
	{
		if(show)
		{
			printf("WB: ");
			printf(formatFrag(frag0), frag0);
			printf(formatFrag(frag1), frag1);
			printf("\n");
		}
		control[0][i] = 1;
		control[1][j] = 1;
		return 1;
	}
	return 0;
}

int tryZB(int i, int j)
{
	if(control[0][i] || control[1][j])
		return 0;
	int frag0 = sets[0][i];
	int frag1 = sets[1][j];
	//
	int c0   = frag0 & C_MASK;
	int c1   = frag1 & C_MASK;
	int w0_0 = frag0 & W0_MASK;
	int w0_1 = frag1 & W0_MASK;
	int w1_0 = frag0 & W1_MASK;
	int w1_1 = frag1 & W1_MASK;
	int q0   = frag0 & Q_MASK;
	int q1   = frag1 & Q_MASK;
	//
	if(q0 == q1)
		return 0;
	if(w1_0 != w1_1)	// different sectors?
		return 0;
	if(w0_0 == w0_1)
		return 0;
	if((c0 ^ c1) & 7)
		return 0;
	//
	if(show)
	{
		printf("ZB: ");
		printf(formatFrag(frag0), frag0);
		printf(formatFrag(frag1), frag1);
		printf("\n");
	}
	control[0][i] = 1;
	control[1][j] = 1;
	return 1;
}

int tryNeutrino(int i, int j)
{
	if(control[0][i] || control[1][j])
		return 0;
	int frag0 = sets[0][i];
	int frag1 = sets[1][j];
	//
	int c0   = frag0 & C_MASK;
	int c1   = frag1 & C_MASK;
	int w0_0 = frag0 & W0_MASK;
	int w0_1 = frag1 & W0_MASK;
	int w1_0 = frag0 & W1_MASK;
	int w1_1 = frag1 & W1_MASK;
	int q0   = frag0 & Q_MASK;
	int q1   = frag1 & Q_MASK;
	//
	if(q0 == q1)
		return 0;
	if(w1_0 != w1_1)
		return 0;
	if(w1_0 != w1_1)
		return 0;
	if(w0_0 != w0_1)
		return 0;
	if(c0 != c1)
		return 0;
	if(c0 != 0 && c0 != 7)
		return 0;
	//
	if(show)
	{
		printf("Neutrino:\t");
		printf(formatFrag(frag0), frag0);
		printf(formatFrag(frag1), frag1);
		printf("\n");
	}
	control[0][i] = 1;
	control[1][j] = 1;
	return 1;
}

int tryGluon(int i, int j)
{
	if(control[0][i] || control[1][j])
		return 0;
	int frag0 = sets[0][i];
	int frag1 = sets[1][j];
	//
	int c0   = frag0 & C_MASK;
	int c1   = frag1 & C_MASK;
	int w0_0 = frag0 & W0_MASK;
	int w0_1 = frag1 & W0_MASK;
	int w1_0 = frag0 & W1_MASK;
	int w1_1 = frag1 & W1_MASK;
	int q0   = frag0 & Q_MASK;
	int q1   = frag1 & Q_MASK;
	//
	if(q0 == q1)						// not neutral?
		return 0;
	if(w1_0 != w1_1)					// different sector?
		return 0;
	if(w0_0 != w0_1)					// different handedness?
		return 0;
	if(w0_0 == w1_0 || w0_1 == w1_1)	// not conjugated?
		return 0;

	// color combination allowed?

	if((c0 == 1 && c1 == 6) ||
	   (c0 == 2 && c1 == 5) ||
	   (c0 == 4 && c1 == 3) ||
	   (c0 == 1 && c1 == 5) ||
	   (c0 == 1 && c1 == 3) ||
	   (c0 == 2 && c1 == 6) ||
	   (c0 == 2 && c1 == 3) ||
	   (c0 == 4 && c1 == 6) ||
	   (c0 == 4 && c1 == 5) ||
	   (c1 == 1 && c0 == 6) ||
	   (c1 == 2 && c0 == 5) ||
	   (c1 == 4 && c0 == 3) ||
	   (c1 == 1 && c0 == 5) ||
	   (c1 == 1 && c0 == 3) ||
	   (c1 == 2 && c0 == 6) ||
	   (c1 == 2 && c0 == 3) ||
	   (c1 == 4 && c0 == 6) ||
	   (c1 == 4 && c0 == 5))
	{
		if(show)
		{
			printf("Gluon: ");
			printf(formatFrag(frag0), frag0);
			printf(formatFrag(frag1), frag1);
			printf("\n");
		}
		control[0][i] = 1;
		control[1][j] = 1;
		return 1;
	}
	return 0;
}

int tryUp(int i, int j)
{
	if(control[0][i] || control[1][j])
		return 0;
	int frag0 = sets[0][i];
	int frag1 = sets[1][j];
	//
	int c0   = frag0 & C_MASK;
	int c1   = frag1 & C_MASK;
	int w0_0 = frag0 & W0_MASK;
	int w0_1 = frag1 & W0_MASK;
	int w1_0 = frag0 & W1_MASK;
	int w1_1 = frag1 & W1_MASK;
	int q0   = frag0 & Q_MASK;
	int q1   = frag1 & Q_MASK;
	//
	if(w1_0 != w1_1)					// different sector?
		return 0;
	if(w0_0 != w0_1)					// different handedness?
		return 0;
	if(w0_0 == w1_0 || w0_1 == w1_1)	// not conjugated?
		return 0;
	if(c0 == c1 && c0 != 0 && c0 != 7 && q0 == q1 && q0 == 0)
	{
		if(show)
		{
			printf("Up: ");
			printf(formatFrag(frag0), frag0);
			printf(formatFrag(frag1), frag1);
			printf("\n");
		}
		control[0][i] = 1;
		control[1][j] = 1;
		return 1;
	}
	return 0;
}

int tryPhoton(int i, int j)
{
	if(control[0][i] || control[1][j])
		return 0;
	int frag0 = sets[0][i];
	int frag1 = sets[1][j];
	//
	int c0   = frag0 & C_MASK;
	int c1   = frag1 & C_MASK;
	int w0_0 = frag0 & W0_MASK;
	int w0_1 = frag1 & W0_MASK;
	int w1_0 = frag0 & W1_MASK;
	int w1_1 = frag1 & W1_MASK;
	int q0   = frag0 & Q_MASK;
	int q1   = frag1 & Q_MASK;
	//
	if(w1_0 != w1_1)	// different sectors?
		return 0;
	if(q0 == q1)		// same charge?
		return 0;
	if(w0_0 == w0_1)	// same handedness?
		return 0;
	if(((c0 ^ c1) & 7) != 7)
		return 0;
	//
	if(show)
	{
		printf("Photon:\t");
		printf(formatFrag(frag0), frag0);
		printf(formatFrag(frag1), frag1);
		printf("\n");
	}
	control[0][i] = 1;
	control[1][j] = 1;
	return 1;
}

void initialize()
{
	for(int s = 0; s < 2; s++)
	{
		for(int i = 0; i < N; i++)
		{
			sets[s][i] = i & 0x3f;
			control[s][i] = 0;
			if(show)
			{
				printf(formatFrag(sets[s][i]), sets[s][i]);
				if(i % 8 == 0)
					printf("\n");
			}
		}
	}
	if(show)
		printf("\nN=%d\n\n", N);
}

void combine()
{
	initialize();
	int n_e = 0;
	int n_q = 0;
	int n_nu = 0;
	int n_gl = 0;
	int n_up = 0;
	int n_ph = 0;
	int n_zb = 0;
	int n_wb = 0;
	for(int n = 0; n < ATTEMPTS; n++)
	{
		int i = rand() % N;
		int j = rand() % N;
		int op = rand() % 8;
		switch(op)
		{
		case 0:
			n_e += tryElectron(i, rand()%2);
			break;
		case 1:
			n_q += tryQuark(i, rand()%2);
			break;
		case 2:
			n_nu += tryNeutrino(i, j);
			break;
		case 3:
			n_gl += tryGluon(i, j);
			break;
		case 4:
			n_up += tryUp(i, j);
			break;
		case 5:
			n_ph += tryPhoton(i, j);
			break;
		case 6:
			n_zb += tryZB(i, j);
			break;
		case 7:
			n_wb += tryWB(i, j);
			break;
		}
	}
	t_e += n_e;
	t_q += n_q;
	t_nu += n_nu;
	t_gl += n_gl;
	t_up += n_up;
	t_ph += n_ph;
	t_zb += n_zb;
	t_wb += n_wb;
	if(show)
	{
		printf("\n\nn_nu=%d n_gl=%d n_up=%d n_ph=%d n_zb=%d n_wb=%d\n\n", n_nu, n_gl, n_up, n_ph, n_zb, n_wb);
		printf("\n\nn_e=%d n_q=%d\n\n", n_e, n_q);
		printf("Total=%d\n", 2 * (n_nu + n_gl + n_up + n_ph + n_zb + n_wb));
	}
}

void leftover()
{
	int n = 0;
	int npO = 0, nnO = 0, npD = 0, nnD = 0;
	if(show)
		puts("Leftover:\n");
	for(int i = 0; i < N; i++)
	{
		if(!control[0][i])
		{
			int frag = sets[0][i];
			int w1   = (frag & W1_MASK)>>4;
			int q    = (frag & Q_MASK)>>5;
			if(w1 == 0 && q == 0)
				npO++;
			else if(w1 == 0 && q == 1)
				nnO++;
			else if(w1 == 1 && q == 0)
				npD++;
			else if(w1 == 1 && q == 1)
				nnD++;
			if(show)
			{
				printf(formatFrag(frag), frag);
				printf("\n");
			}
			n++;
		}
	}
	for(int i = 0; i < N; i++)
	{
		if(!control[1][i])
		{
			int frag = sets[1][i];
			int w1   = (frag & W1_MASK)>>4;
			int q    = (frag & Q_MASK)>>5;
			if(w1 == 0 && q == 0)
				npO++;
			else if(w1 == 0 && q == 1)
				nnO++;
			else if(w1 == 1 && q == 0)
				npD++;
			else if(w1 == 1 && q == 1)
				nnD++;
			if(show)
			{
				printf(formatFrag(frag), frag);
				printf("\n");
			}
			n++;
		}
	}
	if(show)
		printf("Leftover: %d / %d\n", n, N);
	t_lo += n;
	tpO += npO;
	tnO += nnO;
	tpD += npD;
	tnD += nnD;
}

#define TOTAL 100.0

int main(void)
{
	srand(time(0));
	setvbuf(stdout, NULL, _IONBF, 0);
	printf("\n-- ZOO --\n\nN=%d ATTEMPTS=%d NR=%d\n\n", 2*N, ATTEMPTS, (int)TOTAL);
	for(int i = 0; i < TOTAL; i++)
	{
		combine();
		leftover();
		if(show)
			puts("--------------------------");
		else
			putchar('.');
	}
	printf("\n\n[n_nu]=%.2f [n_gl]=%.2f [n_up]=%.2f [n_ph]=%.2f [n_zb]=%.2f [n_wb]=%.2f\n", t_nu / TOTAL, t_gl / TOTAL, t_up / TOTAL, t_ph / TOTAL, t_zb / TOTAL, t_wb / TOTAL);
	printf("[n_e]=%.2f [n_q]=%.2f\n", t_e / TOTAL, t_q / TOTAL);
	printf("[leftover]=%.2f\n", t_lo / TOTAL);
	printf("O[+]=%.1f, O[-]=%.1f, D[+]=%.1f, D[-]=%.1f\n", tpO/TOTAL, tnO/TOTAL, tpD/TOTAL, tnD/TOTAL);
	printf("[N]=%.2f\n", (2*t_nu + 2*t_gl + 2*t_up + 2*t_ph + 2*t_zb + 2*t_wb + t_e + t_q + t_lo) / TOTAL);
	return 0;
}
