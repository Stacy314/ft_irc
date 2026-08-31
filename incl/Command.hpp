#pragma once

#include <string>
#include <vector>

class Command {
private:
    std::string              _name;
    std::vector<std::string> _parameters;
public:
    Command();
    Command(const std::string &name, const std::vector<std::string> &parameters);
    Command(const Command &other);
    Command &operator=(const Command &other);
    ~Command();
    const std::string &getName() const;
    const std::vector<std::string> &getParameters() const;
};
