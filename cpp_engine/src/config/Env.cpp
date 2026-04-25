#include "config/Env.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::map<std::string, std::string> loadEnv(const std::string& path) {
    std::map<std::string, std::string> env;
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open file: " << path << std::endl;
    }

    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string key, value;

        if (std::getline(ss, key, '=') && std::getline(ss, value)) {
            env[key] = value;
        }
    }

    return env;
}