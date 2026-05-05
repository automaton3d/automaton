#include "config.h"
#include <fstream>
#include <sstream>

Config gConfig = {
    { true, false, false, false, false, true, false, true, false }, // data3D
    { false, false, false } // delays
};

static bool parseBool(const std::string& v)
{
    return (v == "1" || v == "true" || v == "TRUE");
}

bool loadConfig(const std::string& path)
{
    std::ifstream file(path);
    if (!file) return false;

    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '/' || line[0] == '#')
            continue;

        std::istringstream iss(line);
        std::string key, eq, value;

        if (!(iss >> key >> eq >> value))
            continue;

        if (key == "data3D.wavefront") gConfig.data3D[0] = parseBool(value);
        if (key == "data3D.momentum")  gConfig.data3D[1] = parseBool(value);
        if (key == "data3D.spin")      gConfig.data3D[2] = parseBool(value);
        if (key == "data3D.sine_mask") gConfig.data3D[3] = parseBool(value);
        if (key == "data3D.hunting")   gConfig.data3D[4] = parseBool(value);
        if (key == "data3D.centers")   gConfig.data3D[5] = parseBool(value);
        if (key == "data3D.lattice")   gConfig.data3D[6] = parseBool(value);
        if (key == "data3D.axes")      gConfig.data3D[7] = parseBool(value);
        if (key == "data3D.plane")     gConfig.data3D[8] = parseBool(value);


        if (key == "delays.convol")  gConfig.delays.convol  = parseBool(value);
        if (key == "delays.diffuse") gConfig.delays.diffuse = parseBool(value);
        if (key == "delays.reloc")   gConfig.delays.reloc   = parseBool(value);
    }

    return true;
}