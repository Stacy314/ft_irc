#include "../incl/Channel.hpp"

Channel::Channel(const std::string& channelName) : name(channelName),
	userLimit(0), inviteOnly(false), protectedTopic(false), counter(1) {}

Channel::~Channel() {}

std::map<Client*, memberInfo>::iterator Channel::getMember(Client* memb)
	{ return members.find(memb); }

std::map<Client*, memberInfo>::const_iterator Channel::getMember(Client* memb) const
	{ return members.find(memb); }

bool	Channel::isEmpty() const { return (members.empty()); }

size_t	Channel::size() const { return members.size(); }

bool	Channel::isMember(Client* memb) const
	{ return (getMember(memb) != members.end()); }

bool	Channel::isOperator(Client* memb) const
{
	std::map<Client*, memberInfo>::const_iterator it = getMember(memb);
	return (it != members.end() && it->second.isOp);
}

bool	Channel::isValidKey(const std::string& key) const
{
	return (this->key.empty() || this->key == key);
}

bool	Channel::isInvited(Client* memb) const
{
	std::set<std::string>::const_iterator it = invite.find(memb->getNick());
	return (it != invite.end());
}

ChannelResult	Channel::setOperator(Client* memb, bool status) //more things to check
{
	if (!isMember(memb))
		return NOT_ON_CHANNEL;

	getMember(memb)->second.isOp = status;
	return SUCCESS;
}

ChannelResult   Channel::removeMember(Client* memb) //also a lot of things to check
{
	if (!isMember(memb))
		return NOT_ON_CHANNEL;
	members.erase(memb);
	return SUCCESS;
}

ChannelResult   Channel::kickMember(Client* actor, Client* victim) // need to add more checks
{
	if (!isMember(actor))
		return NOT_ON_CHANNEL;
	if (getMember(actor)->second.isOp == false)
		return NOT_OPERATOR;
	return (removeMember(victim));
}

ChannelResult   Channel::inviteMember(Client* host, Client* invited) // need to add more checks
{
	if (!isMember(host))
		return NOT_ON_CHANNEL;
	if (getMember(host)->second.isOp == false)
		return NOT_OPERATOR;
	invite.insert(invited->getNick());
	return SUCCESS;
}

ChannelResult   Channel::setInviteOnly(bool status, Client* memb)
{
	if (!isMember(memb))
		return NOT_ON_CHANNEL;
	if (getMember(memb)->second.isOp == false)
		return NOT_OPERATOR;
	inviteOnly = status;
	return SUCCESS;
}

ChannelResult   Channel::setProtectedTopic(bool status, Client* memb)
{
	if (!isMember(memb))
		return NOT_ON_CHANNEL;
	if (getMember(memb)->second.isOp == false)
		return NOT_OPERATOR;
	protectedTopic = status;
	return SUCCESS;
}

ChannelResult   Channel::setUserLimit(size_t limit, Client* memb)
{
	if (!isMember(memb))
		return NOT_ON_CHANNEL;
	if (getMember(memb)->second.isOp == false)
		return NOT_OPERATOR;
	userLimit = limit;
	return SUCCESS;
}

ChannelResult   Channel::setKey(const std::string& key, Client* memb)
{
	if (!isMember(memb))
		return NOT_ON_CHANNEL;
	if (getMember(memb)->second.isOp == false)
		return NOT_OPERATOR;
	this->key = key;
	return SUCCESS;
}

ChannelResult	Channel::addMember(Client* newMemb, const std::string& key)
{
	//initializing operator status && counter
	memberInfo info;
	info.userId = counter;
	if (isEmpty())
		info.isOp = true;	
	else
		info.isOp = false;
	
	//verifying if the key is valid
	if (!this->key.empty() && !isValidKey(key))
		return BAD_KEY;

	//checking if the channel is full
	if (userLimit > 0 && size() >= userLimit)
		return CHANNEL_FULL;
	
	//checking if the channel is invite only and if the new member is invited
	if (inviteOnly && !isInvited(newMemb))
		return INVITE_ONLY;

	//trying to add a new member
	std::pair<std::map<Client*, memberInfo>::iterator, bool> res
		= members.insert(std::make_pair(newMemb, info));
	if (!res.second)
		return ALREADY_MEMBER;
	
	//erasing from invite list if existed
	invite.erase(newMemb->getNick());

	//increment Id
	counter++;
	return SUCCESS;
}