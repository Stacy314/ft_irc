#include "../incl/Channel.hpp"

Channel::Channel(const std::string& channelName) : name(channelName),
	userLimit(0), inviteOnly(false), protectedTopic(false), counter(1) {}

Channel::~Channel() {}

std::map<Client*, MemberInfo>::iterator Channel::findMember(Client* memb)
	{ return members.find(memb); }

std::map<Client*, MemberInfo>::const_iterator Channel::findMember(Client* memb) const
	{ return members.find(memb); }

bool	Channel::isEmpty() const { return (members.empty()); }

size_t	Channel::size() const { return members.size(); }

void	Channel::ensureOperator()
{
	std::map<Client*, MemberInfo>::iterator it
		= members.begin();
	std::map<Client*, MemberInfo>::iterator newOp
		= members.begin();
	while (it != members.end())
	{
		if (it->second.isOp)
			return ;
		else if (it->second.userId < newOp->second.userId)
			newOp = it;
		++it;
	}
	newOp->second.isOp = true;
}

bool	Channel::isMember(Client* memb) const
	{ return (findMember(memb) != members.end()); }

bool	Channel::isOperator(Client* memb) const
{
	std::map<Client*, MemberInfo>::const_iterator it
		= findMember(memb);
	return (it != members.end() && it->second.isOp);
}

bool	Channel::acceptsKey(const std::string& key) const
{
	return (this->key.empty() || this->key == key);
}

bool	Channel::isInvited(Client* memb) const
{
	std::set<std::string>::const_iterator it
		= invite.find(memb->getNick());
	return (it != invite.end());
}

ChannelResult	Channel::accessCheck(Client* actor)
{
	if (!isMember(actor))
		return NOT_ON_CHANNEL;
	if (!isOperator(actor))
		return NOT_OPERATOR;

	return SUCCESS;
}

ChannelResult	Channel::accessCheck(Client* actor, Client* target)
{
	returnCode = accessCheck(actor);
	if (returnCode != SUCCESS)
		return returnCode;

	if (!isMember(target))
		return NOT_IN_CHANNEL;

	return SUCCESS;
}

ChannelResult	Channel::setOperator(Client* actor,
	Client* target, bool status)
{
	returnCode = accessCheck(actor, target);
	if (returnCode != SUCCESS)
		return returnCode;

	std::map<Client*, MemberInfo>::iterator trgt
		= findMember(target);
	if (trgt->second.isOp != status)
	{
		trgt->second.isOp = status;
		if (!status)
			ensureOperator();
		return SUCCESS;
	}
	return NO_CHANGE;
}

ChannelResult   Channel::removeMember(Client* memb)
{
	members.erase(memb);

	if (isEmpty())
		return CHANNEL_EMPTY;
	else
		ensureOperator();

	return SUCCESS;
}

ChannelResult   Channel::kickMember(Client* actor, Client* victim)
{
	returnCode = accessCheck(actor, victim);
	if (returnCode != SUCCESS)
		return returnCode;

	return (removeMember(victim));
}

ChannelResult   Channel::inviteMember(Client* actor, Client* invited)
{
	returnCode = accessCheck(actor);
	if (returnCode != SUCCESS)
		return returnCode;

	invite.insert(invited->getNick());
	return SUCCESS;
}

ChannelResult   Channel::setInviteOnly(bool status, Client* actor)
{
	returnCode = accessCheck(actor);
	if (returnCode != SUCCESS)
		return returnCode;

	inviteOnly = status;
	return SUCCESS;
}

ChannelResult   Channel::setProtectedTopic(bool status, Client* actor)
{
	returnCode = accessCheck(actor);
	if (returnCode != SUCCESS)
		return returnCode;

	protectedTopic = status;
	return SUCCESS;
}

ChannelResult	Channel::setTopic(const std::string& newTopic, Client* actor)
{
	if (protectedTopic)
	{
		returnCode = accessCheck(actor);
		if (returnCode != SUCCESS)
			return returnCode;
	}
	else if (!isMember(actor))
		return NOT_ON_CHANNEL;
	
	topic = newTopic;
	if (topic.empty())
		return NO_TOPIC;
	return SUCCESS;
}

ChannelResult   Channel::setUserLimit(size_t limit, Client* actor)
{
	returnCode = accessCheck(actor);
	if (returnCode != SUCCESS)
		return returnCode;

	userLimit = limit;
	return SUCCESS;
}

ChannelResult   Channel::setKey(const std::string& key, Client* actor)
{
	returnCode = accessCheck(actor);
	if (returnCode != SUCCESS)
		return returnCode;

	this->key = key;
	return SUCCESS;
}

ChannelResult	Channel::addMember(Client* newMemb, const std::string& key)
{
	//initializing operator status && counter
	MemberInfo info;
	info.userId = counter;
	if (isEmpty())
		info.isOp = true;	
	else
		info.isOp = false;
	
	//verifying if the key is valid
	if (!this->key.empty() && !acceptsKey(key))
		return BAD_KEY;

	//checking if the channel is full
	if (userLimit > 0 && size() >= userLimit)
		return CHANNEL_FULL;
	
	//checking if the channel is invite only and if the new member is invited
	if (inviteOnly && !isInvited(newMemb))
		return INVITE_ONLY;

	//trying to add a new member
	std::pair<std::map<Client*, MemberInfo>::iterator, bool> res
		= members.insert(std::make_pair(newMemb, info));
	if (!res.second)
		return ALREADY_MEMBER;
	
	//erasing from invite list if existed
	invite.erase(newMemb->getNick());

	//increment Id
	counter++;
	return SUCCESS;
}