#pragma once

#include <string>
#include <set>
#include <map>
#include "Client.hpp"
#include <cstddef>

enum ChannelResult
{
	SUCCESS,
	NOT_ON_CHANNEL,
    ALREADY_MEMBER,
	NOT_OPERATOR,
    BAD_KEY,
    INVITE_ONLY,
    CHANNEL_FULL
};

struct memberInfo
{
    bool isOp;
    size_t userId;
};

class Channel
{
    private:
        std::string						name;
        std::map<Client*, memberInfo>	members;
        std::set<std::string>			invite;
        std::string						topic;
        std::string						key;
        size_t							userLimit;
        bool							inviteOnly;
        bool							protectedTopic;
        size_t							counter;

        Channel(const Channel&);
        Channel& operator=(const Channel&);
        Channel();

		std::map<Client*, memberInfo>::iterator			getMember(Client* memb);
		std::map<Client*, memberInfo>::const_iterator	getMember(Client* memb) const;
		bool											isValidKey(const std::string& key) const;
		bool											isInvited(Client* memb) const;
    public:
        Channel(const std::string&);
        ~Channel();
        bool            isEmpty() const;
        size_t          size() const;
        bool            isMember(Client*) const;
        bool            isOperator(Client*) const;
        ChannelResult			setOperator(Client*, bool);
		ChannelResult	setTopic(const std::string&, Client*);
        ChannelResult   addMember(Client*, const std::string& key);
		ChannelResult   removeMember(Client*); //QUIT
		ChannelResult   kickMember(Client*, Client*); //KICK
		ChannelResult   inviteMember(Client*, Client*); //INVITE
		ChannelResult   setInviteOnly(bool, Client*); //MODE +i/-i
		ChannelResult   setProtectedTopic(bool, Client*); //MODE +t/-t
		ChannelResult   setUserLimit(size_t, Client*); //MODE +l/-l
		ChannelResult   setKey(const std::string&, Client*);
};