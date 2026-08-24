#pragma once 

#include "Command.hpp"
#include "CommandUtils.hpp"

#include <cctype>
#include <vector>
#include <string>

class Parser {
private:
    Parser();
    Parser(const Parser &other);
    Parser &operator=(const Parser &other);
	~Parser();
public:
    static Command parse(const std::string &line);
};
