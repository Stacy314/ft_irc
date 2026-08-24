#pragma once 

#include "Client.hpp"

#include <string>
#include <map>
#include <vector>
#include <sstream>

std::string toUpper(const std::string &s);
std::string numToString(int n);
void skipSpaces(const std::string &line, std::size_t &position);
bool isNickSpecial(char c);
bool isValidNickname(const std::string &nickname);
