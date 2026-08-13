#pragma once
#include <string>
#include <vector>

struct Message {
    std::string prefix;
    std::string command;
    std::vector<std::string> params;
};

Message parseLine(const std::string& line);
