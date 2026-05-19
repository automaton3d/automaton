// geometry.h
#ifndef GEOMETRY_H
#define GEOMETRY_H

namespace automaton
{
    // Distância geodésica na esfera
    unsigned geodesicDistance(int x, int y, int z);
    
    // Verifica se ponto está dentro da esfera
    bool isInsideSphere(int x, int y, int z);
    
    // Reloca todas as células aleatoriamente (debug)
    void relocateAllWRandom();
    
    // Wrapping antípoda
    void spherical_wrap(int& x, int& y, int& z);
}

#endif // GEOMETRY_H