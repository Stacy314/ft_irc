#include "../incl/Channel.hpp"

Channel::Channel(const std::string& channelName)
{
	name = channelName;
	userLimit = 0;
	inviteOnly = false;
	protectedTopic = false;
	counter = 1;
}

Channel::~Channel() {}

bool	Channel::isEmpty() const { return (!members.size()); }

size_t	Channel::size() const { return members.size(); }

bool	Channel::isMember(Client* memb) const
{
	std::map<Client*, memberInfo>::const_iterator it
		= members.find(memb);
	return (it != members.end());
}

bool	Channel::isOperator(Client* memb) const
{
	std::map<Client*, memberInfo>::const_iterator it
		= members.find(memb);
	if (it != members.end())
		return false;
	return it->second.isOp;
}

bool	Channel::setOperator(Client* memb)
{
	if (!isMember(memb))
		return false;
	std::map<Client*, memberInfo>::iterator it
		= members.find(memb);
	it->second.isOp = true;
	return true;
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