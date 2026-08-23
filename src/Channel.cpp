#include "../incl/Channel.hpp"

Channel::Channel(const std::string& channelName) : name(channelName),
	userLimit(0), inviteOnly(false), protectedTopic(false), counter(1) {}

Channel::~Channel() {}

Channel::Channel(const Channel &other)
    : name(other.name),
      members(other.members),
      invite(other.invite),
      topic(other.topic),
      key(other.key),
      userLimit(other.userLimit),
      inviteOnly(other.inviteOnly),
      protectedTopic(other.protectedTopic),
      counter(other.counter)
{}

Channel &Channel::operator=(const Channel &other)
{
    if (this != &other)
    {
        name = other.name;
        members = other.members;
        invite = other.invite;
        topic = other.topic;
        key = other.key;
        userLimit = other.userLimit;
        inviteOnly = other.inviteOnly;
        protectedTopic = other.protectedTopic;
        counter = other.counter;
    }

    return *this;
}

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

bool	Channel::correctKey(const std::string& key) const
{
	return (this->key == key);
}

bool	Channel::hasKey() const
{
	return (!key.empty());
}

bool	Channel::isInvited(Client* memb) const
{
	std::set<std::string>::const_iterator it
		= invite.find(memb->getNickname());
	return (it != invite.end());
}

ChannelResult	Channel::requireAccess(Client* actor, bool needOp) const
{
	if (!isMember(actor))
		return NOT_ON_CHANNEL;
	if (needOp)
		if (!isOperator(actor))
			return NOT_OPERATOR;
	return SUCCESS;
}

ChannelResult	Channel::requireTarget(Client* target) const
{
	if (!isMember(target))
		return NOT_IN_CHANNEL;

	return SUCCESS;
}

bool	Channel::isFull() const
{
	return (userLimit > 0 && size() >= userLimit);
}

const std::string&	Channel::getTopic() const
{
	return topic;
}

ChannelResult	Channel::setOperator(Client* actor,
	Client* target, bool status)
{
	ChannelResult returnCode = requireAccess(actor, true);
	if (returnCode != SUCCESS)
		return returnCode;
	returnCode = requireTarget(target);
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

ChannelResult   Channel::removeMember(Client* target)
{
	/* A disconnected client can be only invited, not a member. */
	invite.erase(target->getNickname());

	ChannelResult returnCode = requireTarget(target);
	if (returnCode != SUCCESS)
		return returnCode;
	members.erase(target);

	if (isEmpty())
		return CHANNEL_EMPTY;
	else
		ensureOperator();

	return SUCCESS;
}

ChannelResult   Channel::kickMember(Client* actor, Client* victim)
{
	ChannelResult returnCode = requireAccess(actor, true);
	if (returnCode != SUCCESS)
		return returnCode;

	return (removeMember(victim));
}

ChannelResult   Channel::inviteMember(Client* actor, Client* target)
{
	ChannelResult returnCode = requireAccess(actor, inviteOnly);
	if (returnCode != SUCCESS)
		return returnCode;
	if (isMember(target))
		return ALREADY_MEMBER;

	invite.insert(target->getNickname());
	return SUCCESS;
}

ChannelResult   Channel::setInviteOnly(Client* actor, bool status)
{
	ChannelResult returnCode = requireAccess(actor, true);
	if (returnCode != SUCCESS)
		return returnCode;

	if (inviteOnly == status)
		return NO_CHANGE;
	inviteOnly = status;
	return SUCCESS;
}

ChannelResult   Channel::setProtectedTopic(Client* actor, bool status)
{
	ChannelResult returnCode = requireAccess(actor, true);
	if (returnCode != SUCCESS)
		return returnCode;

	if (status == protectedTopic)
		return NO_CHANGE;
	protectedTopic = status;
	return SUCCESS;
}

ChannelResult	Channel::setTopic(Client* actor, const std::string& newTopic)
{
	ChannelResult returnCode = requireAccess(actor, protectedTopic);
	if (returnCode != SUCCESS)
		return returnCode;
	
	topic = newTopic;
	return SUCCESS;
}

ChannelResult   Channel::setUserLimit(Client* actor, size_t limit)
{
	ChannelResult returnCode = requireAccess(actor, true);
	if (returnCode != SUCCESS)
		return returnCode;

	if (userLimit == limit)
		return NO_CHANGE;
	userLimit = limit;
	return SUCCESS;
}

ChannelResult   Channel::setKey(Client* actor, const std::string& key)
{
	ChannelResult returnCode = requireAccess(actor, true);
	if (returnCode != SUCCESS)
		return returnCode;

	if (this->key == key)
		return NO_CHANGE;
	this->key = key;
	return SUCCESS;
}

ChannelResult	Channel::addMember(Client* target, const std::string& key)
{
	//initializing operator status && counter
	MemberInfo info;
	info.userId = counter;
	if (isEmpty())
		info.isOp = true;	
	else
		info.isOp = false;

	//verifying that its a new member
	if (isMember(target))
		return ALREADY_MEMBER;

	//verifying if the key is valid
	if (hasKey() && !correctKey(key))
		return BAD_KEY;

	//checking if the channel is full
	if (isFull())
		return CHANNEL_FULL;
	
	//checking if the channel is invite only and if the new member is invited
	if (inviteOnly && !isInvited(target))
		return INVITE_ONLY;

	//trying to add a new member
	members.insert(std::make_pair(target, info));
	
	//erasing from invite list if existed
	invite.erase(target->getNickname());

	//increment Id
	counter++;
	return SUCCESS;
}

const std::string&	Channel::getName() const { return name; }

std::vector<Client*>	Channel::getMembers() const
{
	std::vector<Client*> list;
	list.reserve(members.size());
	std::map<Client*, MemberInfo>::const_iterator it = members.begin();
	while (it != members.end())
	{
		list.push_back(it->first);
		++it;
	}
	return list;
}

bool	Channel::hasTopic() const { return (!topic.empty()); }

std::string	Channel::getMode(Client* actor) const
{
	std::string res = "+";

	if (inviteOnly)
		res.append("i");
	if (protectedTopic)
		res.append("t");
	if (hasKey() && isMember(actor))
		res.append("k");
	if (userLimit > 0)
		res.append("l");

	if (hasKey() && isMember(actor))
		res.append(" " + key);

	if (userLimit > 0)
		res.append(" " + numToString(userLimit));
	return res;
}
