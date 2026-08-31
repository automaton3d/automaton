/*
 * dispersion.cpp — dispersion relation of the lattice wave field.
 *
 * Replicates the exact integer u/v update of phase_step (simulation.cpp,
 * src/model/simulation.cpp) and measures the temporal frequency omega(k)
 * of plane-wave modes of wave number k on the periodic cubic lattice.
 * For a Lorentz-consistent low-energy limit the dispersion must be linear,
 * omega ~= v_wave * k, at small k (lattice "phonon" regime), with the
 * wave speed set by the integer diffShift of the update:
 *
 *   lap(u) = -lambda * u,   lambda = 4 [ sin^2(kx/2)+sin^2(ky/2)+sin^2(kz/2) ]
 *   v' = v + (lap >> s);  u' = u + v';   v' -= (v' >> velDampShift)
 *   ->  cos(omega) = 1 - lambda / 2^(s+1)
 *   ->  small k:  omega ~= sqrt(1/2^s) * k   (wave speed = 1/2^(s/2))
 *
 * Usage:
 *   dispersion.exe [L] [DIFFSHIFT] [NFRAMES] [VELDAMPSHIFT]
 *     L         lattice side (modes k = 2 pi n / L), default 32
 *     DIFFSHIFT integer shift of the update (model uses ~4..7), default 5
 *     NFRAMES   frames evolved per k, default 8192
 *     VELDAMPSHIFT velocity damping shift (model uses 5; use >=12 for a
 *                 nearly undamped measurement), default 16
 */

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>

int main(int argc, char** argv)
{
  int L        = (argc > 1) ? atoi(argv[1]) : 32;
  int diffShift = (argc > 2) ? atoi(argv[2]) : 5;
  int NFRAMES  = (argc > 3) ? atoi(argv[3]) : 8192;
  int velDampShift = (argc > 4) ? atoi(argv[4]) : 16;   // 16 ~ undamped

  if (L < 2 || NFRAMES < 16)
  {
    fprintf(stderr, "L>=2 and NFRAMES>=16\n");
    return 1;
  }

  const long N = (long)L * L * L;
  std::vector<int> u(static_cast<size_t>(N), 0);
  std::vector<int> v(static_cast<size_t>(N), 0);
  std::vector<int> lap(static_cast<size_t>(N), 0);

  const double PI = 3.14159265358979323846;
  const double kmin = 2.0 * PI / L;

  printf("=== dispersion omega(k) of the lattice wave field ===\n");
  printf("L=%d diffShift=%d velDampShift=%d frames=%d\n",
         L, diffShift, velDampShift, NFRAMES);
  printf("small-k analytic wave speed = 1/2^(s/2) = %.4f cells/frame\n",
         1.0 / std::pow(2.0, diffShift / 2.0));
  printf("  n    k        omega_num   omega_an   omega_num/k\n");
  printf("----------------------------------------------------\n");

  double ratioLow = 0.0;   // omega/k averaged over the smallest 3 modes
  int ratioN = 0;

  for (int n = 1; n <= L / 2; ++n)
  {
    const double k = kmin * n;

    // Standing plane wave along x (u = A cos(k x), v = 0).  A is large so
    // the integer truncation of (lap >> diffShift) is negligible.
    const double A = 1048576.0;
    for (int x = 0; x < L; ++x)
      for (int y = 0; y < L; ++y)
        for (int z = 0; z < L; ++z)
        {
          const long i = ((long)x * L + y) * L + z;
          u[i] = static_cast<int>(A * std::cos(k * x));
          v[i] = 0;
        }

    // Evolve and count sign changes of u at cell (0,0,0).
    long sign = 0;
    int prev = (u[0] >= 0) ? 1 : -1;
    for (int f = 0; f < NFRAMES; ++f)
    {
      for (int x = 0; x < L; ++x)
        for (int y = 0; y < L; ++y)
          for (int z = 0; z < L; ++z)
          {
            const int xm = (x + L - 1) % L, xp = (x + 1) % L;
            const int ym = (y + L - 1) % L, yp = (y + 1) % L;
            const int zm = (z + L - 1) % L, zp = (z + 1) % L;
            const long i  = ((long)x  * L + y ) * L + z;
            lap[i] = u[((long)xp * L + y ) * L + z]
                   + u[((long)xm * L + y ) * L + z]
                   + u[((long)x  * L + yp) * L + z]
                   + u[((long)x  * L + ym) * L + z]
                   + u[((long)x  * L + y ) * L + zp]
                   + u[((long)x  * L + y ) * L + zm]
                   - 6 * u[i];
          }
      for (long i = 0; i < N; ++i)
      {
        int vn = v[i] + (lap[i] >> diffShift);   // exact phase_step arithmetic
        int un = u[i] + vn;
        vn -= (vn >> velDampShift);
        u[i] = un;
        v[i] = vn;
      }
      const int cur = (u[0] >= 0) ? 1 : -1;
      if (cur != prev) { ++sign; prev = cur; }
    }

    const double omegaNum = PI * (double)sign / (double)NFRAMES;
    const double lam      = 4.0 * std::pow(std::sin(k / 2.0), 2.0);
    const double omegaAn  = std::acos(1.0 - lam / std::pow(2.0, diffShift + 1));
    const double ratio    = (k > 0.0) ? omegaNum / k : 0.0;

    printf("%3d  %6.3f  %9.5f  %9.5f  %8.5f\n", n, k, omegaNum, omegaAn, ratio);
    if (n <= 3) { ratioLow += ratio; ++ratioN; }
  }

  printf("----------------------------------------------------\n");
  if (ratioN > 0)
    printf("small-k omega/k (mean over n=1..3) = %.5f  (analytic wave speed %.4f)\n",
           ratioLow / ratioN, 1.0 / std::pow(2.0, diffShift / 2.0));
  return 0;
}
