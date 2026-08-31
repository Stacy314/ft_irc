#include "../incl/CommandUtils.hpp"

std::string toUpper(const std::string &s) {
    std::string r = s;
    for (size_t i = 0; i < r.size(); ++i)
        r[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(r[i])));
    return r;
}

std::string numToString(int n) {
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

bool isValidNickname(const std::string &nickname) {
    if (!std::isalpha(static_cast<unsigned char>(nickname[0])) && !isNickSpecial(nickname[0])) 
        return false;
    
    for (std::size_t i = 1; i < nickname.size(); ++i) {
        if (!std::isalnum(static_cast<unsigned char>(nickname[i]))
            && nickname[i] != '-' && !isNickSpecial(nickname[i])) 
            return false;
    }
    return true;
}
