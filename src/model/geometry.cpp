// geometry.cpp
// Geometric helper functions for spherical topology with antipodal wrapping
// These functions may use floating-point internally, but return integer results
// for deterministic CA behavior.

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdio>
#include <cassert>
#include <random>
#include <vector>
#include <algorithm>
#include "model/simulation.h"
#include "model/geometry.h"

namespace automaton
{
    /**
     * Calculates geodesic distance on a sphere with antipodal wrapping.
     * For points inside the sphere: distance = arcsin(r/R) * R
     * For points outside: distance = πR - arcsin(R/r) * R (path through antipode)
     * 
     * @param x,y,z Coordinates in cube space [0, EL-1]
     * @return Geodesic distance from center (0 to RMAX)
     */
    unsigned geodesicDistance(int x, int y, int z)
    {
        const double R = EL / 2.0;           // Sphere radius
        const double C = (EL - 1) / 2.0;     // Center coordinate
        
        double dx = x - C;
        double dy = y - C;
        double dz = z - C;
        
        double euclidean_r = sqrt(dx*dx + dy*dy + dz*dz);
        
        if (euclidean_r <= 1e-6) {
            return 0;                       // Exact center
        }
        
        if (euclidean_r <= R) {
            // Inside sphere: arc from center to point
            double theta = asin(euclidean_r / R);
            return (unsigned)round(R * theta);
        } else {
            // Outside sphere: path through antipode
            double theta = 3.14159265358979323846 - asin(R / euclidean_r);
            return (unsigned)round(R * theta);
        }
    }
    
    /**
     * Checks whether a point lies inside the spherical domain.
     * Uses a small tolerance for boundary cases.
     * 
     * @param x,y,z Coordinates in cube space [0, EL-1]
     * @return true if inside or on sphere surface
     */
    bool isInsideSphere(int x, int y, int z)
    {
        const double R = EL / 2.0;           // Sphere radius
        const double C = (EL - 1) / 2.0;     // Center coordinate
        
        double dx = x - C;
        double dy = y - C;
        double dz = z - C;
        
        return (dx*dx + dy*dy + dz*dz <= R*R + 0.5);
    }
    
    /**
     * Antipodal spherical wrapping for torus-to-sphere topology.
     * When a point exits the sphere, it re-enters from the antipodal side.
     * This preserves geodesic distance across the sphere's surface.
     * 
     * Algorithm:
     * 1. Keep points inside/on surface unchanged
     * 2. For outside points: project radially to surface, then invert through center
     * 
     * @param x,y,z Coordinates to wrap (modified in place)
     */
    void spherical_wrap(int& x, int& y, int& z)
    {
        const double R = EL / 2.0;           // Sphere radius
        const double C = (EL - 1) / 2.0;     // Center coordinate
        
        double dx = x - C;
        double dy = y - C;
        double dz = z - C;
        
        double r = sqrt(dx*dx + dy*dy + dz*dz);
        
        // Inside or on surface: keep as is
        if (r <= R + 0.5) {
            // Clamp to valid range
            if (x < 0) x = 0;
            if (x >= (int)EL) x = (int)EL - 1;
            if (y < 0) y = 0;
            if (y >= (int)EL) y = (int)EL - 1;
            if (z < 0) z = 0;
            if (z >= (int)EL) z = (int)EL - 1;
            return;
        }
        
        // Outside sphere: project to surface then invert (antipodal)
        double scale = R / r;
        
        int nx = (int)round(dx * scale);
        int ny = (int)round(dy * scale);
        int nz = (int)round(dz * scale);
        
        // Antipodal inversion (through the center)
        nx = -nx;
        ny = -ny;
        nz = -nz;
        
        x = nx + (int)round(C);
        y = ny + (int)round(C);
        z = nz + (int)round(C);
        
        // Final bounds clamping
        if (x < 0) x = 0;
        if (x >= (int)EL) x = (int)EL - 1;
        if (y < 0) y = 0;
        if (y >= (int)EL) y = (int)EL - 1;
        if (z < 0) z = 0;
        if (z >= (int)EL) z = (int)EL - 1;
    }
    
    /**
     * DEBUG: Relocates all cells by a random offset.
     * Used for testing topological consistency.
     * Applies spherical wrap to keep cells within domain.
     */
    void relocateAllWRandom()
    {
        std::vector<Cell> temp = lattice_curr;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<unsigned> dis(0, EL - 1);
        
        unsigned dx = dis(gen);
        unsigned dy = dis(gen);
        unsigned dz = dis(gen);
        
        printf("relocateAllWRandom: shifting by (%u, %u, %u)\n", dx, dy, dz);
        
        for (unsigned w = 0; w < W_USED; ++w)
        {
            for (unsigned x = 0; x < EL; ++x)
            {
                for (unsigned y = 0; y < EL; ++y)
                {
                    for (unsigned z = 0; z < EL; ++z)
                    {
                        // Skip cells outside sphere
                        if (!isInsideSphere((int)x, (int)y, (int)z))
                            continue;
                            
                        const Cell& src = getCell(lattice_curr, x, y, z, w);
                        
                        int nx = (int)x + (int)dx;
                        int ny = (int)y + (int)dy;
                        int nz = (int)z + (int)dz;
                        
                        // Apply antipodal wrapping
                        spherical_wrap(nx, ny, nz);
                        
                        if (isInsideSphere(nx, ny, nz))
                        {
                            Cell& dst = getCell(temp, (unsigned)nx, (unsigned)ny, (unsigned)nz, w);
                            dst.pB = src.pB;
                            dst.sB = src.sB;
                            dst.a = src.a;
                            dst.phiB = src.phiB;
                            dst.t = src.t;
                            dst.f = src.f;
                            dst.d = src.d;
                            dst.s2B = src.s2B;
                            dst.kB = src.kB;
                            dst.bB = src.bB;
                            dst.hB = src.hB;
                            dst.cB = src.cB;
                        }
                    }
                }
            }
        }
        
        lattice_curr.swap(temp);
        puts("relocateAllWRandom ok.");
    }
}