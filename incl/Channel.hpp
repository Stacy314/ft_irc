#pragma once

#include <string>
#include <set>
#include <map>
#include "Client.hpp"
#include <cstddef>

enum ChannelResult
{
	// success codes
	SUCCESS,
    NO_CHANGE,
    CHANNEL_EMPTY,
    // ---------------------------
	// error codes
	NO_TOPIC, //331
	NOT_IN_CHANNEL, //441, TARGET
    NOT_ON_CHANNEL, //442, ACTOR
    ALREADY_MEMBER, //443, TARGET
	NOT_OPERATOR, // 482, ACTOR
    CHANNEL_FULL, // 471
    INVITE_ONLY, //473
    BAD_KEY //475
};

struct MemberInfo
{
    bool isOp;
    size_t userId;
};

class Channel
{
    private:
        std::string						name;
        std::map<Client*, MemberInfo>	members;
        std::set<std::string>			invite;
        std::string						topic;
        std::string						key;
        size_t							userLimit;
        bool							inviteOnly;
        bool							protectedTopic;
        size_t							counter;
		ChannelResult					returnCode;

        Channel(const Channel&);
        Channel& operator=(const Channel&);
        Channel();

		std::map<Client*, MemberInfo>::iterator			findMember(Client*);
		std::map<Client*, MemberInfo>::const_iterator	findMember(Client*) const;
		bool											acceptsKey(const std::string&) const;
		bool											isInvited(Client*) const;
        void                                            ensureOperator();
		ChannelResult									accessCheck(Client*);
		ChannelResult									accessCheck(Client*, Client*);
    public:
        Channel(const std::string&);
        ~Channel();
        size_t          	size() const;
        bool            	isEmpty() const;
        bool            	isMember(Client*) const;
        bool            	isOperator(Client*) const;
		bool				hasKey() const;
		bool				isFull() const;
		const std::string&	getTopic(std::string&) const;
        ChannelResult		setOperator(Client*, Client*, bool);
		ChannelResult		setTopic(const std::string&, Client*);
        ChannelResult   	addMember(Client*, const std::string&);
		ChannelResult   	removeMember(Client*); //QUIT
		ChannelResult   	kickMember(Client*, Client*); //KICK
		ChannelResult   	inviteMember(Client*, Client*); //INVITE
		ChannelResult   	setInviteOnly(bool, Client*); //MODE +i/-i
		ChannelResult   	setProtectedTopic(bool, Client*); //MODE +t/-t
		ChannelResult   	setUserLimit(size_t, Client*); //MODE +l/-l
		ChannelResult   	setKey(const std::string&, Client*);
};