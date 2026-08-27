/*
 * virada.c -- toy probe for "baryogenesis by the turnaround" in the discrete
 * toy universe ("It from bit", charge-conjugation study).
 *
 * The manuscript already hooks a charge inversion at the turnaround
 * (t == RMAX -> an M/M-bar mixture, fsm.md sect.10).  The antipodal wrap
 * (the point 2*CENTER - x) is a *mirror/parity* inversion of the sphere around
 * the centre, so inverting charge there composes into a C*P-type operation.
 *
 * Premise probed here (stated by the author):
 *   - all bubbles are born superposed at the centre (initCenters), so the
 *     early phase is a dense "hot" clump;
 *   - in that clump almost no bubble makes a full 0->RMAX trip "virgem"
 *     (untouched); interactions collapse-and-reissue (reset the clock) long
 *     before the antipode;
 *   - as the bubbles separate, the virgin fraction grows;
 *   - at the wrap (turnover) charges are inverted.
 * Question: can this produce a net matter/antimatter asymmetry in Orbis and
 * (mutatis mutandis) for Umbra?
 *
 * Honest structural result this file demonstrates against a C-violating knob:
 *   eps = 0  (C-conjugated interactions):  inversion alone gives ZERO net
 *            asymmetry, in both sectors -- the fewer-virgin premise by itself
 *            does nothing.
 *   eps != 0 (matter and antimatter interact at different rates, breaking C):
 *            a net signed imbalance appears, which scales with eps and with
 *            how many trips actually wrap while the "hot" early phase lasts.
 *            Its sign follows the sign of eps.  Measured placement (80 laps):
 *            - mode 0 (sector-preserving inversion) -> coherent same-sign
 *              excess in both sectors, dOrb ~ dUmb ~ global/2;
 *            - mode 1 (constrained full-C that also flips the sector w1) ->
 *              same global excess; the sector swap moves bubbles across
 *              sectors, yet each sector settles with the SAME sign
 *              (dOrb = dUmb exactly).
 *
 * The charge word matches combine.c: bit5 = q (electric), bit4 = w1
 * (sector Orbis/Umbra), bit3 = w0 (chirality), bits 2..0 = color.
 * Only q = w1 xor w0 is valid (32 of 64 words); the full complement ~c is
 * NOT a valid state, which re-derives the combine.c conservation bug.
 *
 * Build:  cl /nologo /O2 /Fe:virada.exe virada.c
 * Run:    virada
 */

#include <stdio.h>
#include <math.h>

#define CH(q, w1, w0, col)  (((q) << 5) | ((w1) << 4) | ((w0) << 3) | (col))
#define Q(c)   (((c) >> 5) & 1)
#define W1(c)  (((c) >> 4) & 1)
#define W0(c)  (((c) >> 3) & 1)
#define COL(c) ((c) & 7)
#define SIG(c) ((((c) >> 2) & 1) + (((c) >> 1) & 1) + ((c) & 1))  /* colour weight */
#define IS_MAT(col) (SIG(col) < 2)          /* matter colours: N,R,G,B */

#define GROUPS     6
#define PER_GROUP  32768
#define N_BUB      (GROUPS * PER_GROUP)     /* 196608 */
#define VALID      32

static unsigned int rng_state = 2463534242u;
static unsigned int xrand(void)
{
    unsigned x = rng_state;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return (rng_state = x);
}
static double frand(void) { return (double)(xrand() >> 8) / (double)(1u << 24); }

/* Apply the charge inversion (restricted to the valid set) to word c.
   mode 0 : invert electric charge + colour, KEEP sector (w1) and chirality (w0).
   mode 1 : also flip sector w1 (and q, so q = w1^w0 stays valid); colour inverted. */
static unsigned invert_charge(unsigned c, int mode)
{
    int q = Q(c), w1 = W1(c), w0 = W0(c);
    int ncol = (int)((~COL(c)) & 7);
    if (mode == 0)
        return CH(1 - q, w1, w0, ncol);      /* in-sector inversion    */
    else
        return CH(1 - q, 1 - w1, w0, ncol);  /* cross-sector inversion */
}

/* virgin survival chance of one trip vs the lap number: tiny in the early
   clump (almost all re-emitted), -> 1 once the universe has thinned. */
static double virgin_of(int lap)
{
    double tau = 12.0;
    return 1.0 - exp(-(double)lap / tau);
}

/* materiality label driving the C-violation (matter vs antimatter).
   mode 0/1 : the same signed coupling in both sectors (global C-break).
   mode 2   : anti-symmetric coupling between sectors (mirror C/CP): a bubble
              sees a reversed C-bias after crossing w1.  EMPIRICAL RESULT:
              washout to noise (~0) -- after each wrap BOTH colour and sector
              conjugate, so the bias is invariant along the bubble's own
              trajectory and the M<->A rates stay symmetric.
   mode 3   : handedness anchored to an *immutable* birth attribute H[i]
              (the discrete analogue of "each bubble keeps its W address
              forever", fsm sect.8).  EMPIRICAL RESULT: also washout to
              noise -- a fixed label makes the flip rate state-independent,
              i.e. the M<->A chain is symmetric and relaxes to 50/50.
              Only modes 0/1 work: there the rate out of the matter state
              differs from the rate out of the antimatter state, because the
              coupling reads the CURRENT colour.  That state-dependence is
              the whole mechanism (see the feedback analysis below main). */
static int csc(unsigned c, int mode)
{
    int s = IS_MAT(COL(c)) ? +1 : -1;           /* + matter / - antimatter */
    if (mode == 2)
        return (W1(c) == 0) ? s : -s;           /* sector-mirror coupling   */
    return s;
}
static unsigned char ch[N_BUB];   /* current charge word of each bubble */
static signed char  H[N_BUB];     /* immutable birth handedness (+1/-1) */
static double nwrap;              /* cumulative virgin wraps (flips applied) */

int main(void)
{
    int mode;
    double eps;

    printf("turnaround probe  N=%d  laps/run=80\n", N_BUB);

    for (mode = 0; mode <= 3; mode++)   /* 0/1 global C, 2 mirror-CP, 3 anchored H */
    {
        for (eps = -0.3; eps <= 0.301 + 1e-9; eps += 0.1)
        {
            int i, lap;
            long matO=0, antiO=0, matU=0, antiU=0;

            rng_state = 2463534242u;        /* identical stream per cell */
            nwrap = 0.0;

            /* charge-balanced start: each valid word gets N/VALID copies */
            for (i = 0; i < N_BUB; i++) {
                int w = i % VALID;
                int w1 = (w >> 4) & 1;
                int w0 = (w >> 3) & 1;
                int col = w & 7;
                ch[i] = (unsigned char)CH(w1 ^ w0, w1, w0, col);
                H[i]  = (signed char)IS_MAT(col) ? +1 : -1;   /* fixed at birth */
            }

            for (lap = 1; lap <= 80; lap++)
            {
                double pbase = virgin_of(lap);
                for (i = 0; i < N_BUB; i++) {
                    unsigned char cc = ch[i];
                    double bias = (mode == 3) ? (double)H[i] : (double)csc(cc, mode);
                    double p = pbase * (1.0 + eps * bias);
                    if (p > 0.999999) p = 0.999999;
                    if (p < 0.0)      p = 0.0;
                    if (frand() < p) {
                        ch[i] = (unsigned char)invert_charge(cc, mode);
                        nwrap += 1.0;
                    }
                }
            }

            for (i = 0; i < N_BUB; i++) {
                unsigned char cc = ch[i];
                int ismat = IS_MAT(COL(cc));
                if (W1(cc) == 0) { if (ismat) matO++; else antiO++; }
                else             { if (ismat) matU++; else antiU++; }
            }

            double dO = (double)(matO - antiO);
            double dU = (double)(matU - antiU);
            double glob = dO + dU;
            printf("mode %d eps=%4.2f | wraps=%.0f | "
                   "dOrb=%+7.0f dUmb=%+7.0f   global=%+7.0f\n",
                   mode, eps, nwrap, dO, dU, glob);
        }
        printf("--------------------------------------------------------------\n");
    }

    /* combine.c reminder: the full six-bit complement is never a valid word. */
    {
        int valid_full = 0, w1, w0, col;
        for (w1 = 0; w1 < 2; w1++)
            for (w0 = 0; w0 < 2; w0++)
                for (col = 0; col < 8; col++) {
                    int c  = CH(w1 ^ w0, w1, w0, col);
                    int cc = (~c) & 0x3F;
                    int qb = (cc >> 5) & 1, w1b = (cc >> 4) & 1, w0b = (cc >> 3) & 1;
                    if (qb == (w1b ^ w0b)) valid_full++;
                }
        printf("full-complement partner exists for %d of %d valid words "
               "(=> physical C must be constrained)\n", valid_full, VALID);
    }
/* Feedback (colour-current) mechanism -- analytic vs measured.
       Why mode 0/1 yield a robust asymmetry:
       recursion   M' = M(1-r_m) + A r_a ,   A' = A(1-r_a) + M r_m
       with        r_m = pbase(1+eps) , r_a = pbase(1-eps).
       Fixed point: M/A = r_a/r_m = (1-eps)/(1+eps)
       => fractional asymmetry (M-A)/N -> signed +/-eps.
       We compare measured per-lap asymmetry to that, mode 0 eps=+0.3. */
    {
        int i, lap;
        double eps = 0.30;

        rng_state = 2463534242u;
        for (i = 0; i < N_BUB; i++) {
            int w = i % VALID;
            int w1 = (w >> 4) & 1, w0 = (w >> 3) & 1, col = w & 7;
            ch[i] = (unsigned char)CH(w1 ^ w0, w1, w0, col);
        }

        printf("\nfeedback analysis (mode 0, eps=%4.2f)  [colour-current feedback]\n", eps);
        printf("lap   pbase      ham-matter   ham-anti   asymfrac  theory  M/A\n");
        for (lap = 1; lap <= 40; lap++) {
            double pbase = virgin_of(lap);
            long nM = 0, nA = 0;
            int i2;
            for (i2 = 0; i2 < N_BUB; i2++) {
                if (IS_MAT(COL(ch[i2]))) nM++; else nA++;
            }
            double terror = -eps;
            double ratio  = (nA > 0) ? ((double)nM / (double)nA) : 0.0;
            printf("%3d %5.3f  %7ld %7ld  %+7.4f  %+7.4f  %6.3f\n",
                   lap, pbase, nM, nA, (double)(nM - nA) / (double)N_BUB, terror, ratio);

            /* advance to next lap with the state-dependent flip */
            for (i2 = 0; i2 < N_BUB; i2++) {
                unsigned char cc = ch[i2];
                double bias = IS_MAT(COL(cc)) ? +1.0 : -1.0;
                double p = pbase * (1.0 + eps * bias);
                if (p > 0.999999) p = 0.999999;
                if (p < 0.0)      p = 0.0;
                if (frand() < p) ch[i2] = (unsigned char)invert_charge(cc, 0);
            }
        }
    }

    return 0;
}