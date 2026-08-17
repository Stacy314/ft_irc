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
		return 442;

	getMember(memb)->second.isOp = status;
	return 0;
}

ChannelResult   Channel::removeMember(Client* memb) //also a lot of things to check
{
	if (!isMember(memb))
		return 442;
	members.erase(memb);
	return 0;
}

ChannelResult   Channel::kickMember(Client* actor, Client* victim) // need to add more checks
{
	if (!isMember(actor))
		return 442;
	if (getMember(actor)->second.isOp == false)
		return 482;
	return (removeMember(victim));
}

ChannelResult   Channel::inviteMember(Client* host, Client* invited) // need to add more checks
{
	if (!isMember(host))
		return 442;
	if (getMember(host)->second.isOp == false)
		return 482;
	invite.insert(invited->getNick());
	return 0;
}

ChannelResult   Channel::setInviteOnly(bool status, Client* memb)
{
	if (!isMember(memb))
		return 442;
	if (getMember(memb)->second.isOp == false)
		return 482;
	inviteOnly = status;
	return 0;
}

ChannelResult   Channel::setProtectedTopic(bool status, Client* memb)
{
	if (!isMember(memb))
		return 442;
	if (getMember(memb)->second.isOp == false)
		return 482;
	protectedTopic = status;
	return 0;
}

ChannelResult   Channel::setUserLimit(size_t limit, Client* memb)
{
	if (!isMember(memb))
		return 442;
	if (getMember(memb)->second.isOp == false)
		return 482;
	userLimit = limit;
	return 0;
}

ChannelResult   Channel::setKey(const std::string& key, Client* memb)
{
	if (!isMember(memb))
		return 442;
	if (getMember(memb)->second.isOp == false)
		return 482;
	this->key = key;
	return 0;
}

ChannelResult	Channel::addMember(Client* newMemb)
{
	//initializing operator status && counter
	memberInfo info;
	info.userId = counter;
	if (isEmpty())
		info.isOp = true;	
	else
		info.isOp = false;
	
	//checking if the channel is invite only and if the new member is invited
	if (inviteOnly && !isInvited(newMemb))
		return 442;

	//trying to add a new member
	std::pair<std::map<Client*, memberInfo>::iterator, bool> res
		= members.insert(std::make_pair(newMemb, info));
	if (!res.second)
		return 443;
	
	//erasing from invite list if existed
	invite.erase(newMemb->getNick());

	//increment Id
	counter++;
	return 0;
}