#pragma once
#include <array>
#include <string>

struct Config
{
    bool data3D[9];

    struct {
        bool convol;
        bool diffuse;
        bool reloc;
    } delays;
};

extern Config gConfig;

// load/save
bool loadConfig(const std::string& path);