#pragma once

#include "CommandUtils.hpp"
#include "Client.hpp"

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

		std::map<Client*, MemberInfo>::iterator			findMember(Client*);
		std::map<Client*, MemberInfo>::const_iterator	findMember(Client*) const;
		bool											correctKey(const std::string&) const;
		bool											isInvited(Client*) const;
        void                                            ensureOperator();
		ChannelResult									requireAccess(Client*, bool) const;
		ChannelResult									requireTarget(Client*) const;
        Channel();
    public:
		Channel& operator=(const Channel&);
		Channel(const Channel&);
        Channel(const std::string&);
        ~Channel();
        size_t          	  size() const;
        bool            	  isEmpty() const;
        bool            	  isMember(Client*) const;
        bool            	  isOperator(Client*) const;
		bool				  isFull() const;
		bool				  hasKey() const;
        bool                  hasTopic() const;
		const std::string&	  getTopic() const;
        const std::string&    getName() const;
        std::string           getMode(Client*) const; //MODE no arguments
        std::vector<Client*>  getMembers() const;
        ChannelResult		  setOperator(Client*, Client*, bool);
		ChannelResult   	  setInviteOnly(Client*, bool); //MODE +i/-i
		ChannelResult   	  setProtectedTopic(Client*, bool); //MODE +t/-t
		ChannelResult   	  setUserLimit(Client*, size_t); //MODE +l/-l
		ChannelResult   	  setKey(Client*, const std::string&); //MODE +k
		ChannelResult		  setTopic(Client*, const std::string&);
        ChannelResult   	  addMember(Client*, const std::string&);
		ChannelResult   	  inviteMember(Client*, Client*); //INVITE
		ChannelResult   	  removeMember(Client*); //QUIT
		ChannelResult   	  kickMember(Client*, Client*); //KICK
};