#ifndef ENV_H
#define ENV_H

#include <string>
#include <map>

std::map<std::string, std::string> loadEnv(const std::string& path);

#endif