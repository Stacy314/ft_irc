#include "../incl/Parser.hpp"

Parser::Parser() {}

Parser::Parser(const Parser &other) {(void)other;}

Parser &Parser::operator=(const Parser &other) {
    (void)other;
    return *this;
}

Parser::~Parser() {}

Command Parser::parse(const std::string &line) {
    std::size_t position = 0;
    std::string commandName;
    std::vector<std::string> parameters;
    skipSpaces(line, position);
    std::size_t commandStart = position;
    while (position < line.length() && line[position] != ' ')
        ++position;
    commandName = line.substr(commandStart, position - commandStart);
    commandName = toUpper(commandName);
    while (position < line.length()) {
        skipSpaces(line, position);
        if (position >= line.length())
            break;
        if (line[position] == ':') {
            ++position;
            parameters.push_back(line.substr(position));
            break;
        }
        std::size_t parameterStart = position;
        while (position < line.length() && line[position] != ' ')
            ++position;
        parameters.push_back(line.substr(parameterStart, position - parameterStart));
    }
    return Command(commandName, parameters);
}