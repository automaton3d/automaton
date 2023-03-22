/*
 ============================================================================
 Name        : color.c
 Author      : Alexandre
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N         (6 * 4086)
//#define N         (6 * 8192)
//#define N         (6 * 65536)
#define C_MASK    0x07
#define W0_MASK   0x08
#define W1_MASK   0x10
#define Q_MASK    0x20

#define REPEAT    60.0

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

long t_e, t_e_D;
long t_q, t_q_D;

long t_nu;
long t_gl;
long t_up;
long t_ph, t_ph_D;
long t_PH;
long t_zb;
long t_wb;

int tpO, tnO, tpD, tnD;

long t_lo;

int bonus;	// used to match up quarks with electrons

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

int attempts = 30000000;

void trySwap(int i, int j)
{
	if(control[0][i] || control[1][j])
		return;
	int frag0 = sets[0][i];
	int frag1 = sets[1][j];
	//
	int c0 = frag0 & C_MASK;
	int c1 = frag1 & C_MASK;
	if(c0 == 0 || c0 == 7 || c1 == 0 || c1 == 7)
		return;
	if((c0 ^ c1) == 7)
	{
		int val = 7 * (rand() % 2);
		sets[0][i] = val;
		sets[1][j] = val ^7;
		sets[0][j] = val ^7;
		sets[1][i] = val;
	}
}

void trySwap2(int i, int j)
{
	if(control[0][i] || control[1][j])
		return;
	int frag0 = sets[0][i];
	int frag1 = sets[1][j];
	//
	int c0 = frag0 & C_MASK;
	int c1 = frag1 & C_MASK;
	if(c0 == 0 || c0 == 7 || c1 == 0 || c1 == 7)
		return;
	int mask = rand() % 7;
	sets[0][i] ^= mask;
	sets[1][j] ^= mask;
	sets[0][j] ^= mask;
	sets[1][i] ^= mask;
}

int tryElectron(int i, int j)
{
	if(control[j][i] == 0 && bonus)
	{
		for(int k = 0; k < 4; k++)
		{
			int frag = sets[j][i];
			if(frag == electrons[k])
			{
				int w1 = (frag & W1_MASK)>>4;
				control[j][i] = 1;
				if(w1)
					t_e_D++;
				bonus--;
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
			int frag = sets[j][i];
			if(frag == quarks[k])
			{
				int w1 = (frag & W1_MASK)>>4;
				control[j][i] = 1;
				if(w1)
					t_q_D++;
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
	int w0_0 = (frag0 & W0_MASK)>>3;
	int w0_1 = (frag1 & W0_MASK)>>3;
	int w1_0 = (frag0 & W1_MASK)>>4;
	int w1_1 = (frag1 & W1_MASK)>>4;
	int q0   = (frag0 & Q_MASK)>>5;
	int q1   = (frag1 & Q_MASK)>>5;
	//
	if(q0 != q1 || w1_0 != w1_1 || w0_0 != w0_1)
		return 0;
	if(q0 != w0_0)
		return 0;
	if((c0 == 0 && c1 == 7) || (c0 == 7 && c1 == 0))
	{
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
	int w0_0 = (frag0 & W0_MASK)>>3;
	int w0_1 = (frag1 & W0_MASK)>>3;
	int w1_0 = (frag0 & W1_MASK)>>4;
	int w1_1 = (frag1 & W1_MASK)>>4;
	int q0   = (frag0 & Q_MASK)>>5;
	int q1   = (frag1 & Q_MASK)>>5;
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
	int w0_0 = (frag0 & W0_MASK)>>3;
	int w0_1 = (frag1 & W0_MASK)>>3;
	int w1_0 = (frag0 & W1_MASK)>>4;
	int w1_1 = (frag1 & W1_MASK)>>4;
	int q0   = (frag0 & Q_MASK)>>5;
	int q1   = (frag1 & Q_MASK)>>5;
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
	int w0_0 = (frag0 & W0_MASK)>>3;
	int w0_1 = (frag1 & W0_MASK)>>3;
	int w1_0 = (frag0 & W1_MASK)>>4;
	int w1_1 = (frag1 & W1_MASK)>>4;
	int q0   = (frag0 & Q_MASK)>>5;
	int q1   = (frag1 & Q_MASK)>>5;
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
		control[0][i] = 1;
		control[1][j] = 1;
		return 1;
	}
	return 0;
}

float matter = 0;
float antimatter = 0;

int tryUp(int i, int j)
{
	if(control[0][i] || control[1][j])
		return 0;
	int frag0 = sets[0][i];
	int frag1 = sets[1][j];
	//
	int c0   = frag0 & C_MASK;
	int c1   = frag1 & C_MASK;
	int w0_0 = (frag0 & W0_MASK)>>3;
	int w0_1 = (frag1 & W0_MASK)>>3;
	int w1_0 = (frag0 & W1_MASK)>>4;
	int w1_1 = (frag1 & W1_MASK)>>4;
	int q0   = (frag0 & Q_MASK)>>5;
	int q1   = (frag1 & Q_MASK)>>5;
	//
	if(w1_0 != w1_1)					// different sector?
		return 0;
	if(w0_0 != w0_1)					// different handedness?
		return 0;
	if(w0_0 == w1_0 || w0_1 == w1_1)	// not conjugated?
		return 0;
	if(c0 == c1 && c0 != 0 && c0 != 7 && q0 == q1 && q0 == 0)
	{
		bonus += 3;
		control[0][i] = 1;
		control[1][j] = 1;
		if(w1_0 == 0)
			matter++;
		else
			antimatter++;
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
	int w0_0 = (frag0 & W0_MASK)>>3;
	int w0_1 = (frag1 & W0_MASK)>>3;
	int w1_0 = (frag0 & W1_MASK)>>4;
	int w1_1 = (frag1 & W1_MASK)>>4;
	int q0   = (frag0 & Q_MASK)>>5;
	int q1   = (frag1 & Q_MASK)>>5;
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
	if(w1_0)
		t_ph_D++;
	control[0][i] = 1;
	control[1][j] = 1;
	return 1;
}

int tryPHOTON(int i, int j)
{
	if(control[0][i] || control[1][j])
		return 0;
	int frag0 = sets[0][i];
	int frag1 = sets[1][j];
	//
	int c0   = frag0 & C_MASK;
	int c1   = frag1 & C_MASK;
	int w0_0 = (frag0 & W0_MASK)>>3;
	int w0_1 = (frag1 & W0_MASK)>>3;
	//int w1_0 = (frag0 & W1_MASK)>>4;
	//int w1_1 = (frag1 & W1_MASK)>>4;
	int q0   = (frag0 & Q_MASK)>>5;
	int q1   = (frag1 & Q_MASK)>>5;
	//
	if(q0 == q1)		// same charge?
		return 0;
	if(w0_0 == w0_1)	// same handedness?
		return 0;
	if(((c0 ^ c1) & 7) != 7)
		return 0;
	//
	control[0][i] = 1;
	control[1][j] = 1;
	return 1;
}

void initialize()
{
	bonus = 0;
	for(int s = 0; s < 2; s++)
	{
		for(int i = 0; i < N; i++)
		{
			sets[s][i] = i & 0x3f;
			control[s][i] = 0;
			if(0)
			{
				printf(formatFrag(sets[s][i]), sets[s][i]);
				if(i % 8 == 0)
					printf("\n");
			}
		}
	}
}

int nops = 9;

void combine(int init)
{
	if(init)
		initialize();
	//
	int n_e = 0;
	int n_q = 0;
	int n_nu = 0;
	int n_gl = 0;
	int n_up = 0;
	int n_ph = 0;
	int n_PH = 0;
	int n_zb = 0;
	int n_wb = 0;
	for(int n = 0; n < attempts; n++)
	{
		int i = rand() % N;
		int j = rand() % N;
		int op = rand() % nops;
		switch(op)
		{
		case 0:
			n_e += tryElectron(i, j % 2);
			break;
		case 1:
			n_q += tryQuark(i, j % 2);
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
		case 8:
			n_PH += tryPHOTON(i, j);
			break;
		default:
			trySwap(i, j);
		}
	}
	t_e += n_e;
	t_q += n_q;
	t_nu += n_nu;
	t_gl += n_gl;
	t_up += n_up;
	t_ph += n_ph;
	t_PH += n_PH;
	t_zb += n_zb;
	t_wb += n_wb;
}

int leftover(int flg)
{
	int n = 0;
	int npO = 0, nnO = 0, npD = 0, nnD = 0;
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
			n++;
		}
	}
	t_lo += n;
	tpO += npO;
	tnO += nnO;
	tpD += npD;
	tnD += nnD;
	return n;
}

/*
 * Inverts w1 charges if chg1 == ~chg2 of leftover chg1,2
 */
void tiebraker()
{
	int r1[N], r2[N];
	for(int i = 0; i < N; i++)
	{
		r1[i] = 0;
		r2[i] = 0;
	}
	for(int i = 0; i < N; i++)
	{
		if(r1[i] || control[0][i])
			continue;
		for(int j = 0; j < N; j++)
		{
			if(r2[j] || control[1][j])
				continue;
			r1[i] = 1;
			r2[j] = 1;
			if((sets[0][i] ^ sets[1][j]) == 0x3f)
			{
				sets[0][i] ^= W1_MASK;
				sets[1][j] ^= W1_MASK;
			}
		}
	}
}

int last = 100000000;

int ratio_ = 0;

int optimize()
{
	tpO = 0;
	tnO = 0;
	tpD = 0;
	tnD = 0;
	t_e = 0;
	t_e_D = 0;
	t_q = 0;
	t_q_D = 0;
	t_nu = 0;
	t_gl = 0;
	t_up = 0;
	t_ph = 0;
	t_ph_D = 0;
	t_PH = 0;
	t_zb = 0;
	t_wb = 0;
	t_lo = 0;
	printf("\n-- ZOO --\n\nN=%d ATTEMPTS=%d NR=%d\n\n", 2*N, attempts, (int)REPEAT);
	int nleft = 0;
	for(int i = 0; i < REPEAT; i++)
	{
		combine(1);
		tiebraker();
		combine(0);
		nleft += leftover(i);
		putchar('.');
	}
	printf("\n\nFragment\tType\t\tQuantity\n");
	printf("----------------------------------------------------------------------\n");
	int percent = (int)(100 * t_ph_D / (double)t_ph);
	printf("neutrino\tPair\t\t%.2f\ngluon\t\tPair\t\t%.2f\n", t_nu / REPEAT, t_gl / REPEAT);
	printf("up quark\tPair\t\t%.2f %f%%/%f%%\n", t_up / REPEAT, matter/t_up, antimatter/t_up);
	printf("photon\t\tPair\t\t%.2f\t\tO %d%%, D %d%%\n", t_ph / REPEAT, 100 - percent, percent);
	printf("PHoton\t\tPair\t\t%.2f\nZ boson\t\tPair\t\t%.2f\nW boson\t\tPair\t\t%.2f\n", t_PH / REPEAT, t_zb / REPEAT, t_wb / REPEAT);
	percent = (int)(100 * t_e_D / (double)t_e);
	printf("electron\tSingle\t\t%.2f\tO %d%%, D %d%%\n", t_e / REPEAT, 100 - percent, percent);
	percent = (int)(100 * t_q_D / (double)t_q);
	printf("quark\t\tSingle\t\t%.2f\t\tO %d%%, D %d%%\n", t_q / REPEAT, 100-percent, percent);
	printf("leftover\tSingle\t\t%.2f\t\t", t_lo / REPEAT);
	printf("O[+]=%.1f, O[-]=%.1f, D[+]=%.1f, D[-]=%.1f\n", tpO/REPEAT, tnO/REPEAT, tpD/REPEAT, tnD/REPEAT);
	printf("Total\t\t\t\t%.2f\n", (2*t_nu + 2*t_gl + 2*t_up + 2*t_ph + 2*t_PH + 2*t_zb + 2*t_wb + t_e + t_q + t_lo) / REPEAT);
	printf("----------------------------------------------------------------------\n");
	int ratio = (2*t_gl + 2*t_up + 2*t_ph + 2*t_PH + t_q + t_lo) / (3*1836);
	printf("ratio=%d\n", ratio);
	/*
	int tmp = last;
	last = nleft;
	return nleft >= tmp;
	*/
	if(ratio < ratio_)
	{
		ratio_ = ratio;
		return 1;
	}
	ratio_ = ratio;
	return 0;
}

int ____main(void)
{
	//srand(time(0));
	setvbuf(stdout, NULL, _IONBF, 0);
	while(!optimize())
		attempts += 50000;
	return 0;
}


int main(void)
{
	//srand(time(0));
	setvbuf(stdout, NULL, _IONBF, 0);
	while(!optimize())
		nops++;
	return 0;
}
