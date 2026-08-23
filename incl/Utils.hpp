#pragma once 

#include <string>
#include <set>
#include <map>
#include <vector>
#include <cstddef>

std::vector<std::string> split(const std::string &s);
std::string trimCrlf(const std::string &s);
std::string toUpper(const std::string &s);
bool isNumber(const std::string &s);
bool validNick(const std::string &nick);
bool startsWith(const std::string &s, const std::string &prefix);
std::string numToString(int n);
std::string numToString(size_t n);
void skipSpaces(const std::string &line, std::size_t &position);
bool isNickSpecial(char c);

