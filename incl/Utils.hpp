#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>

std::vector<std::string> split(const std::string &s);
std::string trimCrlf(const std::string &s);
std::string toUpper(const std::string &s);
bool isNumber(const std::string &s);
bool validNick(const std::string &nick);
bool startsWith(const std::string &s, const std::string &prefix);
std::string intToString(int n);

#endif
