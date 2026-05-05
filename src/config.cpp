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
        else if (key == "data3D.momentum")  gConfig.data3D[1] = parseBool(value);
        else if (key == "data3D.spin")      gConfig.data3D[2] = parseBool(value);
        else if (key == "data3D.sine_mask") gConfig.data3D[3] = parseBool(value);
        else if (key == "data3D.hunting")   gConfig.data3D[4] = parseBool(value);
        else if (key == "data3D.centers")   gConfig.data3D[5] = parseBool(value);
        else if (key == "data3D.lattice")   gConfig.data3D[6] = parseBool(value);
        else if (key == "data3D.axes")      gConfig.data3D[7] = parseBool(value);
        else if (key == "data3D.plane")     gConfig.data3D[8] = parseBool(value);

        // =========================
        // delays
        // =========================
        else if (key == "delay.convol") {
            gConfig.delays.convol = parseBool(value);
            convol_delay = gConfig.delays.convol;
        }
        else if (key == "delay.diffuse") {
            gConfig.delays.diffuse = parseBool(value);
            diffuse_delay = gConfig.delays.diffuse;
        }
        else if (key == "delay.reloc") {
            gConfig.delays.reloc = parseBool(value);
            reloc_delay = gConfig.delays.reloc;
        }

        // =========================
        // camera / input (novo modelo)
        // =========================
        else if (key == "view.camera_speed")
            gConfig.camera_speed = std::stof(value);

        else if (key == "view.mouse_sensitivity")
            gConfig.mouse_sensitivity = std::stof(value);

        else if (key == "view.zoom")
            gConfig.zoom = std::stof(value);

        // =========================
        // projection
        // =========================
        else if (key == "projection.fov")
            gConfig.fov = std::stof(value);

        else if (key == "projection.near")
            gConfig.near_plane = std::stof(value);

        else if (key == "projection.far")
            gConfig.far_plane = std::stof(value);

        else if (key == "projection.perspective")
            gConfig.perspective = parseBool(value);
    }

    return true;
}