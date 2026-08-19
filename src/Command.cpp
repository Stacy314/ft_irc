#include "../incl/Command.hpp"

Command::Command() {}

Command::Command(const std::string &name, const std::vector<std::string> &parameters) 
: _name(name), _parameters(parameters) {}

Command::Command(const Command &other) : _name(other._name), _parameters(other._parameters) {}

Command &Command::operator=(const Command &other) {
    if (this != &other)     {
        _name = other._name;
        _parameters = other._parameters;
    }
    return *this;
}

Command::~Command() {}

const std::string &Command::getName() const {
    return _name;
}

const std::vector<std::string> &Command::getParameters() const {
    return _parameters;
}