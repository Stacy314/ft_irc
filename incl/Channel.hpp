#pragma once

#include <string>
#include <set>
#include <map>
#include "Client.hpp"
#include <cstddef>

typedef int ChannelResult;

struct memberInfo
{
    bool isOp;
    size_t userId;
};

class Channel
{
    private:
        std::string name;
        std::map<Client*, memberInfo> members;
        std::set<std::string> invite;
        std::string topic;
        std::string key;
        size_t userLimit;
        bool inviteOnly;
        bool protectedTopic;
        size_t counter;
        Channel(const Channel&);
        Channel& operator=(const Channel&);
        Channel();
    public:
        Channel(const std::string&);
        ~Channel();
        bool            isEmpty() const;
        size_t          size() const;
        bool            isMember(Client*) const;
        bool            isOperator(Client*) const;
        bool   setOperator(Client*);
        ChannelResult   addMember(Client*);
};