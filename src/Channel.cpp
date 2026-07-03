#include "../incl/Channel.hpp"

Channel::Channel() : inviteOnly(false), topicOpsOnly(true), userLimit(0) {}

Channel::Channel(const std::string &n)
    : name(n), inviteOnly(false), topicOpsOnly(true), userLimit(0) {}

bool Channel::has(int fd) const { return members.find(fd) != members.end(); }
bool Channel::isOp(int fd) const { return operators.find(fd) != operators.end(); }

std::string Channel::modes() const {
    std::string m = "+";
    if (inviteOnly) m += "i";
    if (topicOpsOnly) m += "t";
    if (!key.empty()) m += "k";
    if (userLimit > 0) m += "l";
    if (m == "+") m = "+";
    return m;
}
