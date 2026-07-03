#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class Channel {
public:
    Channel();
    Channel(const std::string &name);

    std::string name;
    std::string topic;
    std::string key;
    bool inviteOnly;
    bool topicOpsOnly;
    int userLimit;
    std::set<int> members;
    std::set<int> operators;
    std::set<int> invited;

    bool has(int fd) const;
    bool isOp(int fd) const;
    std::string modes() const;
};

#endif
