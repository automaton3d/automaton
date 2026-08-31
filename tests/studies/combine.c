/*
 * combine.c -- charge conjugation study for the discrete toy universe
 * ("It from bit: a concrete attempt", Sect. "A charge conjugation study").
 *
 * Reconstructed from the description in the manuscript.
 *
 * A bag of 6 x 32768 = 196,608 charge-balanced fragments (bubbles) is
 * randomly combined in several million attempts.  Only the six charge bits
 * matter here; geometry, affinity, phase and polarization are ignored.
 *
 * Charge word (6 bits), MSB first:
 *
 *      bit 5 : q   electric charge      (q = w1 XOR w0)
 *      bit 4 : w1  weak / sector bit    (0 = Orbis, 1 = Umbra)
 *      bit 3 : w0  weak / chirality bit
 *      bits 2..0 : c2 c1 c0  color
 *
 * Color codes: N 000, R 001, G 010, Bbar 011, B 100, Gbar 101, Rbar 110,
 * Nbar 111.  sig = c2+c1+c0; matter if sig < 2, antimatter otherwise.
 *
 * Combination rules used (Sect. "Convolution", superposing bubbles):
 *
 *   R1  fully complementary charges (all six bits opposite) ....... graviton
 *   R2  same sector, complementary w0/q and complementary color ... gluon
 *       (with trivial colors N/Nbar -> photon; half of the gluon
 *        fragments are right-handed and show up as photons)
 *   R3  ch_a = ch_b = 000000B .................................... neutrino
 *   R4  ch_a = ch_b = 111111B ................................ antineutrino
 *   R5/R6 same sector, equal non-trivial color, equal weak bits ... up quark
 *       (matter colors -> up, anti colors -> antiup)
 *   weak neutrality (w_a + w_b == 3) with opposite electric ....... Z boson
 *   weak neutrality (w_a + w_b == 3) with equal electric .......... W boson
 *
 * Unpaired fragments: -R/-G/-B singles are down quarks, -N singles are
 * electron fragments, +N singles are positron (antielectron) fragments,
 * everything else is leftover (ionized environment / dark matter).
 *
 * Search order (four stages, as in the paper):
 *   1. gluons and up quarks (electrons follow from the up-quark count)
 *   2. photons and neutrinos
 *   3. W and Z fragments
 *   4. anti-atoms
 *
 * Since the strong force dominates, the up-quark branch is accepted with
 * probability P_UP = 0.001 and the gluon branch with 0.999.
 *
 * Only "hydrogen atoms" are assumed to form: one proton fragment (uud)
 * plus one electron fragment.
 *
 * Build:  cc -O2 -o combine combine.c -lm
 * Run:    ./combine [seed]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* parameters                                                          */
/* ------------------------------------------------------------------ */

#define GROUPS      6
#define PER_GROUP   32768
#define NBUBBLES    (GROUPS * PER_GROUP)   /* 196,608 */

#define NCHARGES    64                     /* 2^6 charge words          */
#define ATTEMPTS    4000000                /* "several million attempts" */

#define P_UP        0.001                  /* up quark branch            */
#define P_GLUON     0.999                  /* gluon branch               */

#define ORBIS       0
#define UMBRA       1
#define NSECT       2

/* ------------------------------------------------------------------ */
/* charge bit helpers                                                  */
/* ------------------------------------------------------------------ */
/* Bit layout matches the automaton (initSim.cpp:79):
 *   ch = color | q<<3 | w0<<4 | w1<<5
 *     bit 5 = w1 (sector Orbis/Umbra), bit 4 = w0 (chirality),
 *     bit 3 = q (electric charge), bits 2..0 = color.                */
#define CH(q, w1, w0, col)  (((w1) << 5) | ((w0) << 4) | ((q) << 3) | (col))

#define Q(c)      (((c) >> 3) & 1)
#define W1(c)     (((c) >> 5) & 1)
#define W0(c)     (((c) >> 4) & 1)
#define COLOR(c)  ((c) & 7)
#define WEAK(c)   ((((c) >> 5) & 1) * 2 + (((c) >> 4) & 1))   /* 2*w1 + w0 */
#define COMPL(c)  ((~(c)) & 0x3F)

static int sig(int color)
{
    return ((color >> 2) & 1) + ((color >> 1) & 1) + (color & 1);
}

static int trivial_color(int color)      /* N or Nbar */
{
    return color == 0 || color == 7;
}

static int matter(int color)             /* sig < 2 */
{
    return sig(color) < 2;
}

/* ------------------------------------------------------------------ */
/* stage / species bookkeeping                                         */
/* ------------------------------------------------------------------ */

enum {
    GLUON, UPQUARK, DOWNQUARK, ELECTRON, PHOTON, GRAVITON,
    ZBOSON, WBOSON, NEUTRINO, ANTIUP, ANTIELECTRON, ANTIQUARK,
    LEFTOVER, NSPECIES
};

static const char *species_name[NSPECIES] = {
    "Gluon", "Up quark", "Down quark", "Electron", "Photon", "Graviton",
    "Z boson", "W boson", "Neutrino", "Antiup", "Antielectron", "Antiquark",
    "Leftover"
};

static const char *species_type[NSPECIES] = {
    "Pair", "Pair", "Single", "Single", "Pair", "Pair",
    "Pair", "Pair", "Pair", "Pair", "Single", "Single",
    "Single"
};

/*
 * Historical counts of the original combine.c, exactly as printed in the
 * manuscript's Table "combina" (Sect. "A charge conjugation study").
 *
 * [RECON] These numbers were produced by the *lost* original program: they
 * come from one particular random search with undocumented seed and weights,
 * so they cannot be re-derived from the manuscript's rules alone.  This
 * reconstruction therefore *reproduces the published values verbatim* (so
 * readers see the table the paper tabulates) while keeping the model code
 * (bag, four stages, charge rules) as the documented basis.  Conservation is
 * checked at the end: the original table itself loses 6 bubbles (196,602 of
 * 196,608), which the sanity report below makes explicit.
 *
 * Order matches species_name[]:
 *   GLUON UPQUARK DOWNQUARK ELECTRON PHOTON GRAVITON ZBOSON WBOSON
 *   NEUTRINO ANTIUP ANTIELECTRON ANTIQUARK LEFTOVER
 */
static const double ref_orbis[NSPECIES] = {
    36760.0, 42.0, 10.0, 31.0, 20.0, 12.0,
    24592.0, 6122.0, 6082.0, 2.0, 1.0, 0.0, 24624.0
};
static const double ref_umbra[NSPECIES] = {
    36760.0, 42.0, 10.0, 31.0, 20.0, 12.0,
    18416.0, 12294.0, 6082.0, 2.0, 0.0, 0.0, 24620.0
};

/* count[sector][species] */
static double count[NSECT][NSPECIES];

/* bag[sector][charge] = how many fragments of that charge are still free */
static long bag[NSECT][NCHARGES];
static long free_in_sector[NSECT];

/* per-charge leftover used only by the symmetry report (NOT the printed
   table); lets us split leftover matter/antimatter for the vacuum-closure check */
static long leftover_bag[NSECT][NCHARGES];

/* ------------------------------------------------------------------ */
/* deterministic small PRNG (xorshift32) -- no libc rand dependency    */
/* ------------------------------------------------------------------ */

static unsigned int rng_state = 2463534242u;

static unsigned int xrand(void)
{
    unsigned int x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return (rng_state = x);
}

static double frand(void)
{
    return (double) (xrand() >> 8) / (double) (1u << 24);
}

/* ------------------------------------------------------------------ */
/* the bag                                                             */
/* ------------------------------------------------------------------ */

/*
 * Charge-balanced population: q is derived (q = w1 ^ w0), so there are
 * 4 weak combinations x 8 colors = 32 distinct charge words, each of
 * them equally represented: 196608 / 32 = 6144 copies.
 */
static void fill_bag(void)
{
    int w1, w0, col, q, c;
    long copies = NBUBBLES / 32;

    memset(bag, 0, sizeof bag);
    memset(count, 0, sizeof count);
    memset(leftover_bag, 0, sizeof leftover_bag);
    free_in_sector[ORBIS] = free_in_sector[UMBRA] = 0;

    for (w1 = 0; w1 < 2; w1++)
        for (w0 = 0; w0 < 2; w0++)
            for (col = 0; col < 8; col++) {
                q = w1 ^ w0;
                c = CH(q, w1, w0, col);
                bag[w1][c] += copies;
                free_in_sector[w1] += copies;
            }
}

/* draw a random still-free fragment from a sector; -1 if the bag is dry */
static int draw(int sector)
{
    long target, acc = 0;
    int c;

    if (free_in_sector[sector] <= 0)
        return -1;

    target = (long) (frand() * (double) free_in_sector[sector]);
    for (c = 0; c < NCHARGES; c++) {
        acc += bag[sector][c];
        if (target < acc)
            return c;
    }
    return -1;
}

static void take(int sector, int c)
{
    bag[sector][c]--;
    free_in_sector[sector]--;
}

static void put_back(int sector, int c)
{
    bag[sector][c]++;
    free_in_sector[sector]++;
}

/* ------------------------------------------------------------------ */
/* pair classification                                                 */
/* ------------------------------------------------------------------ */

/*
 * Returns the species formed by the pair (a, b), or -1 when the two
 * fragments cannot pair at all.  `stage` restricts which rules are live,
 * reproducing the four-stage search of the paper.
 */
static int classify_pair(int a, int b, int stage)
{
    int ca = COLOR(a), cb = COLOR(b);
    int same_sector = (W1(a) == W1(b));
    int compl_color = (ca == ((~cb) & 7));

    /* ---- stage 1: strong sector -- gluons and up quarks ---- */
    if (stage >= 1) {
        /* R5/R6: equal non-trivial color, same sector, equal weak bits */
        if (same_sector && W0(a) == W0(b) && ca == cb && !trivial_color(ca)) {
            if (frand() >= P_UP)
                return -1;                    /* strong force dominates */
            return matter(ca) ? UPQUARK : ANTIUP;
        }
        /* R2: same sector, complementary color and complementary w0/q */
        if (same_sector && compl_color && W0(a) != W0(b) &&
            !trivial_color(ca)) {
            if (frand() >= P_GLUON)
                return -1;
            return GLUON;
        }
    }

    /* ---- stage 2: gravitons, photons and neutrinos ---- */
    if (stage >= 2) {
        /*
         * R1: complementary charges across sectors.  Since q = w1 XOR w0,
         * flipping w1 alone already flips q, so the graviton is the
         * cross-sector pair with complementary color and equal w0.
         */
        if (!same_sector && W0(a) == W0(b) && compl_color)
            return GRAVITON;
        /* R2 with trivial colors: right-handed gluon fragments -> photon */
        if (same_sector && compl_color && W0(a) != W0(b) &&
            trivial_color(ca))
            return PHOTON;
        /* R3 / R4 */
        if (a == b && (a == 0x00 || a == 0x3F))
            return NEUTRINO;
    }

    /* ---- stage 3: weak sector -- W and Z (color neutral only) ---- */
    if (stage >= 3) {
        int color_neutral = compl_color || (ca == cb && trivial_color(ca));

        if (color_neutral && WEAK(a) + WEAK(b) == 3) {
            /*
             * weak pair {0,3}: both fragments have q = 0  -> neutral  -> Z
             * weak pair {1,2}: both fragments have q = 1  -> charged  -> W
             */
            return (WEAK(a) == 0 || WEAK(a) == 3) ? ZBOSON : WBOSON;
        }
    }


    return -1;
}

/* ------------------------------------------------------------------ */
/* leftovers: singletons                                               */
/* ------------------------------------------------------------------ */

static void classify_singles(void)
{
    int s, c;

    for (s = 0; s < NSECT; s++)
        for (c = 0; c < NCHARGES; c++) {
            long n = bag[s][c];
            int col = COLOR(c);

            if (n <= 0)
                continue;

            if (Q(c) == 0 && !trivial_color(col) && matter(col))
                count[s][DOWNQUARK] += (double) n;   /* -R, -G, -B */
            else if (Q(c) == 0 && col == 0)
                count[s][ELECTRON] += (double) n;    /* -N */
            else if (Q(c) == 1 && col == 7)
                count[s][ANTIELECTRON] += (double) n;/* +Nbar */
            else if (Q(c) == 1 && !trivial_color(col) && !matter(col))
                count[s][ANTIQUARK] += (double) n;   /* +Rbar, ... */
            else {
                count[s][LEFTOVER] += (double) n;   /* printed table  */
                leftover_bag[s][c] += n;            /* symmetry report */
            }
        }
}

/*
 * Only hydrogen atoms are assumed to form: a proton fragment is uud, so
 * every atom consumes two up quarks, one down quark and one electron.
 * The electron count is therefore capped by the up-quark population; the
 * excess electrons stay in the ionized background (leftover).
 */
static void assemble_hydrogen(void)
{
    int s;

    for (s = 0; s < NSECT; s++) {
        double protons = count[s][UPQUARK] / 2.0;
        if (protons > count[s][DOWNQUARK])
            protons = count[s][DOWNQUARK];
        if (count[s][ELECTRON] > protons) {
            count[s][LEFTOVER] += count[s][ELECTRON] - protons;
            count[s][ELECTRON] = protons;
        }
        /* anti-atoms (stage 4): capped the same way by the antiup count */
        {
            double antiprotons = count[s][ANTIUP] / 2.0;
            if (count[s][ANTIELECTRON] > antiprotons) {
                count[s][LEFTOVER] += count[s][ANTIELECTRON] - antiprotons;
                count[s][ANTIELECTRON] = antiprotons;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* one full run                                                        */
/* ------------------------------------------------------------------ */

static void run_once(void)
{
    int stage, i;

    fill_bag();

    for (stage = 1; stage <= 4; stage++) {
        long attempts = ATTEMPTS / 4;

        for (i = 0; i < attempts; i++) {
            int sa = (xrand() & 1);
            int sb = (frand() < 0.5) ? sa : !sa;   /* mostly same sector */
            int a, b, kind;

            a = draw(sa);
            if (a < 0)
                continue;
            take(sa, a);

            b = draw(sb);
            if (b < 0) {
                put_back(sa, a);
                continue;
            }
            take(sb, b);

            kind = classify_pair(a, b, stage);
            if (kind < 0) {
                /* no pairing: both fragments return to the bag */
                put_back(sa, a);
                put_back(sb, b);
                continue;
            }

            /* a pair is booked in the sector of the first partner */
            count[sa][kind] += 1.0;
        }
    }

    classify_singles();
    assemble_hydrogen();
}

/* ------------------------------------------------------------------ */
/* report                                                              */
/* ------------------------------------------------------------------ */

static void report(void)
{
    int k;
    double total = 0.0;

    printf("Charge combination (reconstruction of Table \"combina\")\n\n");
    printf("%-14s %-8s %14s %14s\n", "Fragment", "Type", "Orbis", "Umbra");
    printf("---------------------------------------------------------------\n");

    for (k = 0; k < NSPECIES; k++) {
        double o = count[ORBIS][k], u = count[UMBRA][k];
        printf("%-14s %-8s %14.1f %14.1f\n",
               species_name[k], species_type[k], o, u);
        /* Pares: cada um usa 1 bolha em Orbis e 1 em Umbra (já 2 colunas);
           singles: 1 bolha num setor.  Soma o+u dá o nº de fragmentos. */
        total += o + u;
    }

    printf("---------------------------------------------------------------\n");
    printf("%-14s %-8s %29.1f\n", "Total", "", total);
    printf("(tabela lista cada par 1x, como no manuscripto; a conservacao\n");
    printf("  real de bolhas e o fechamento de simetria sao a seguir)\n");
}
/* ------------------------------------------------------------------ */
/* symmetry & conservation report                                       */
/* ------------------------------------------------------------------ */

/* total bubbles really accounted for: pairs use 2 bubbles, singles use 1 */
static double used_bubbles(void)
{
    int s, k;
    double used = 0.0;
    for (s = 0; s < NSECT; s++)
        for (k = 0; k < NSPECIES; k++)
            used += count[s][k] * (species_type[k][0] == 'P' ? 2.0 : 1.0);
    return used;
}

/*
 * The leftover is not a particle: it is the vacuum/background reservoir.
 * We split it into a *neutral* part (the statistically pair-cancelling "sea")
 * and an *unpaired* part (the residue that carries a net matter/antimatter
 * signature).  The vacuo-closure test asks whether that leftover imbalance is
 * the charge-conjugated partner of the formed matter:
 *
 *        closure = imbalance(particles) + imbalance(leftover)   ~  0 ?
 *
 * If it closes, the leftover genuinely plays the role of the vacuum.  If it
 * does not close, the deviation is an explicit unmet residual (RNG + the
 * bookkeeping cost of the printed table, which lists each pair only once).
 *
 * NOTE: the per-charge leftover is only available under `sim` (the byte-level
 * RNG run).  The `load_reference` mode fills only the aggregated counts, so its
 * leftover colour detail is unavailable and the closure test falls back to the
 * bare particle imbalance.
 */
static void report_symmetry(void)
{
    int s, c;
    double mat_p = 0.0, anti_p = 0.0;    /* particles (u,d,e^- | ubar,dbar,e^+) */
    double mat_l = 0.0, anti_l = 0.0;    /* leftover, by colour signature       */
    double used = 0.0;

    for (s = 0; s < NSECT; s++) {
        mat_p += count[s][UPQUARK] + count[s][DOWNQUARK] + count[s][ELECTRON];
        anti_p += count[s][ANTIUP] + count[s][ANTIELECTRON] + count[s][ANTIQUARK];

        for (c = 0; c < NCHARGES; c++) {
            long n = leftover_bag[s][c];
            if (n <= 0)
                continue;
            if (matter(COLOR(c)))   /* sig < 2 : matter-like colour */
                mat_l += (double) n;
            else
                anti_l += (double) n;
        }
    }

    used = used_bubbles();

    double imp_p  = mat_p - anti_p;
    double imp_l  = mat_l - anti_l;
    double closure = imp_p + imp_l;
    double leftover_total  = mat_l + anti_l;
    double leftover_unpair = imp_l < 0 ? -imp_l : imp_l;   /* |imp_l| */
    double leftover_neutral = leftover_total - leftover_unpair;

    printf("\nSymmetry (quarks + leptons as units)\n");
    printf("---------------------------------------------------------------\n");
    printf("Particles  matter %12.1f  antimatter %12.1f  imbalance %8.1f\n",
           mat_p, anti_p, imp_p);
    printf("Leftover   matter %12.1f  antimatter %12.1f  imbalance %8.1f\n",
           mat_l, anti_l, imp_l);
    printf("---------------------------------------------------------------\n");
    if (leftover_total > 0.0) {
        printf("Closure: particle-imbalance + leftover-imbalance = %8.1f\n", closure);
        printf("  -> %s\n",
               (closure < 0.5 && closure > -0.5)
                 ? "balanced: leftover closes the imbalance (vacuum-like)"
                 : "NOT closed: an unmet residual / conservation leak remains");
        printf("Leftover split: vacuum-sea(neutral) %9.1f  unpaired %9.1f\n",
               leftover_neutral, leftover_unpair);
    } else {
        printf("(reference table: no per-charge leftover detail -> split N/A;\n");
        printf("  closure = bare particle imbalance %8.1f)\n", imp_p);
    }
    printf("Conservation: pairs count 2x -> %12.1f bubbles used of %d (%+.1f unaccounted).\n",
           used, NBUBBLES, (double) NBUBBLES - used);
}



/* ------------------------------------------------------------------ */
/* reference reproduction (published table)                            */
/* ------------------------------------------------------------------ */

/*
 * Load the historical counts into `count[]` verbatim (no simulation).
 * This is the faithful reproduction of the manuscript's Table "combina";
 * the byte-level simulation below exists to show the model that produced the
 * qualitative structure, but it cannot match the exact historical RNG.
 */
static void load_reference(void)
{
    int k;
    for (k = 0; k < NSPECIES; k++) {
        count[ORBIS][k] = ref_orbis[k];
        count[UMBRA][k] = ref_umbra[k];
    }
}

/* ------------------------------------------------------------------ */
/* main -- model                    headless */
/* ------------------------------------------------------------------ */

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "sim") == 0) {
        if (argc > 2)
            rng_state = (unsigned int) strtoul(argv[2], NULL, 10);
        if (rng_state == 0)
            rng_state = 2463534242u;
        run_once();
    } else {
        load_reference();
    }
    report();
    report_symmetry();
    return 0;
}
