#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config
{
    // =========================
    // data3D
    // =========================
    bool data3D[9] = {0};

    // =========================
    // delays
    // =========================
    struct {
        bool convol  = false;
        bool diffuse = false;
        bool reloc   = false;
    } delays;

    // =========================
    // view (SIMPLIFICADO)
    // =========================
    float camera_speed      = 1.0f;
    float mouse_sensitivity = 0.1f;
    float zoom              = 45.0f;

    // =========================
    // projection
    // =========================
    float fov         = 45.0f;
    float near_plane  = 0.1f;
    float far_plane   = 1000.0f;
    bool  perspective = true;
};

// global config
extern Config gConfig;

// loader
bool loadConfig(const std::string& path);

#endif