#pragma once
#include <string>
#include <vector>

struct Message {
    std::string prefix;
    std::string command;
    std::vector<std::string> params;
};

namespace Parser {
    Message parseLine(const std::string& line);
}