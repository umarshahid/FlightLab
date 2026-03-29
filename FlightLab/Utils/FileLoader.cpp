#pragma once
#include "INIReader.h"
#include "enums.h"
#include "iostream"
#include "string"
#include "vector"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>


class FileLoader {

    static std::string getExeDir() {
        char buffer[MAX_PATH];
        DWORD len = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) {
            return "";
        }

        std::string fullPath(buffer, len);
        size_t pos = fullPath.find_last_of("\\/");
        if (pos == std::string::npos) {
            return "";
        }

        return fullPath.substr(0, pos);
    }

    static bool fileExists(const std::string& path) {
        DWORD attributes = GetFileAttributesA(path.c_str());
        return (attributes != INVALID_FILE_ATTRIBUTES) &&
            ((attributes & FILE_ATTRIBUTE_DIRECTORY) == 0);
    }

    static std::string resolvePathsIni() {
        const std::string exeDir = getExeDir();
        if (!exeDir.empty()) {
            const std::vector<std::string> candidates = {
                exeDir + "\\assets\\files\\paths.ini",
                exeDir + "\\..\\assets\\files\\paths.ini",
                exeDir + "\\..\\..\\assets\\files\\paths.ini"
            };

            for (const auto& candidate : candidates) {
                if (fileExists(candidate)) {
                    return candidate;
                }
            }
        }

        return "../assets/files/paths.ini";
    }

public:
    static std::string getButtonTexture(SimulationObjectType buttonType) {
        INIReader reader(resolvePathsIni());

        if (reader.ParseError() < 0) {
            std::cerr << "Error: Unable to load 'paths.ini'\n";
            return "default_value";
        }

        switch (buttonType) {
        case SimulationObjectType::Aircraft:
            return reader.Get("BUTTON_ICONS", "aircraft", "default_value");
        case SimulationObjectType::Waypoint:
            return reader.Get("BUTTON_ICONS", "waypoint", "default_value");
        case SimulationObjectType::Unknown:
        default:
            return reader.Get("BUTTON_ICONS", "plan", "default_value");
        }
    }

    static std::string getSimulationObjectTexture(SimulationObjectType buttonType) {
        INIReader reader(resolvePathsIni());

        if (reader.ParseError() < 0) {
            std::cerr << "Error: Unable to load 'paths.ini'\n";
            return "default_value";
        }

        switch (buttonType) {
        case SimulationObjectType::Aircraft:
            return reader.Get("OBJECT_ICONS", "aircraft", "default_value");
        case SimulationObjectType::Waypoint:
            return reader.Get("OBJECT_ICONS", "waypoint", "default_value");
        case SimulationObjectType::Missile:
            return reader.Get("OBJECT_ICONS", "missile", "default_value");
        case SimulationObjectType::Unknown:
        default:
            return reader.Get("OBJECT_ICONS", "plan", "default_value");
        }
    }

    static std::string getMapFile() {
        INIReader reader(resolvePathsIni());

        if (reader.ParseError() < 0) {
            std::cerr << "Error: Unable to load 'paths.ini'\n";
            return "default_value";
        }

        return reader.Get("MAP", "Pakistan", "default_value");
    }
};
