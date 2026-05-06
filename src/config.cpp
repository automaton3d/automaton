#include "config.h"
#include "model/simulation.h"
#include <fstream>
#include <sstream>

Config gConfig;

using namespace automaton;

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

        // =========================
        // data3D
        // =========================
        if (key == "data3D.wavefront") gConfig.data3D[0] = parseBool(value);
        if (key == "data3D.momentum")  gConfig.data3D[1] = parseBool(value);
        if (key == "data3D.spin")      gConfig.data3D[2] = parseBool(value);
        if (key == "data3D.sine_mask") gConfig.data3D[3] = parseBool(value);
        if (key == "data3D.hunting")   gConfig.data3D[4] = parseBool(value);
        if (key == "data3D.centers")   gConfig.data3D[5] = parseBool(value);
        if (key == "data3D.lattice")   gConfig.data3D[6] = parseBool(value);
        if (key == "data3D.axes")      gConfig.data3D[7] = parseBool(value);
        if (key == "data3D.plane")     gConfig.data3D[8] = parseBool(value);

        // =========================
        // delays
        // =========================
        if (key == "delay.convol") {
            gConfig.delays.convol = parseBool(value);
            convol_delay = gConfig.delays.convol;
        }

        if (key == "delay.diffuse") {
            gConfig.delays.diffuse = parseBool(value);
            diffuse_delay = gConfig.delays.diffuse;
        }

        if (key == "delay.reloc") {
            gConfig.delays.reloc = parseBool(value);
            reloc_delay = gConfig.delays.reloc;
        }

        // =========================
        // view
        // =========================
        if (key == "view.zoom")
            gConfig.view.zoom = std::stof(value);

        if (key == "view.vis_dx")
            gConfig.view.vis_dx = std::stoi(value);

        if (key == "view.vis_dy")
            gConfig.view.vis_dy = std::stoi(value);

        if (key == "view.vis_dz")
            gConfig.view.vis_dz = std::stoi(value);

        // =========================
        // projection
        // =========================
        if (key == "projection.fov")
            gConfig.projection.fov = std::stof(value);

        if (key == "projection.near")
            gConfig.projection.near_plane = std::stof(value);

        if (key == "projection.far")
            gConfig.projection.far_plane = std::stof(value);

        if (key == "projection.perspective")
            gConfig.projection.perspective = parseBool(value);
    }

    return true;
}