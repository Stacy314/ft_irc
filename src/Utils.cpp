#include "../incl/Utils.hpp"
#include <sstream>
#include <cctype>

std::vector<std::string> split(const std::string &s) {
    std::vector<std::string> out;
    std::string cur;
    bool trailing = false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (!trailing && s[i] == ' ') {
            if (!cur.empty()) {
                out.push_back(cur);
                cur.clear();
            }
            continue;
        }
        if (!trailing && s[i] == ':') {
            if (!cur.empty()) {
                out.push_back(cur);
                cur.clear();
            }
            cur = s.substr(i + 1);
            out.push_back(cur);
            return out;
        }
        cur += s[i];
    }
    if (!cur.empty())
        out.push_back(cur);
    return out;
}

std::string trimCrlf(const std::string &s) {
    size_t end = s.size();
    while (end > 0 && (s[end - 1] == '\r' || s[end - 1] == '\n'))
        --end;
    return s.substr(0, end);
}

std::string toUpper(const std::string &s) {
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i)
        r[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(r[i])));
    return r;
}

bool isNumber(const std::string &s) {
    if (s.empty())
        return false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return false;
    }
    return true;
}

bool validNick(const std::string &nick) {
    if (nick.empty() || nick.size() > 30)
        return false;
    if (!std::isalpha(static_cast<unsigned char>(nick[0])) && nick[0] != '_' && nick[0] != '[' && nick[0] != ']')
        return false;
    for (size_t i = 0; i < nick.size(); ++i) {
        char c = nick[i];
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-' && c != '[' && c != ']' && c != '\\' && c != '`' && c != '^')
            return false;
    }
    return true;
}

bool startsWith(const std::string &s, const std::string &prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

std::string numToString(int n) {
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

std::string numToString(size_t n) {
    std::ostringstream oss;
    oss << n;
    return oss.str();
}

void skipSpaces(const std::string &line, std::size_t &position) {
    while (position < line.length() && std::isspace(static_cast<unsigned char>(line[position])))
        ++position;
}

bool isNickSpecial(char c) {
    const std::string special = "_[]\\`^{}|";
    return special.find(c) != std::string::npos;
}