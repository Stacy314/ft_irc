#include "../incl/CommandHandler.hpp"
#include "../incl/Server.hpp"
#include "../incl/Utils.hpp"

void CommandHandler::sendNumeric(Client &client, const std::string &code,
                 const std::string &params, const std::string &text)
{
    std::string line = ":" + _server.serverName() + " " + code + " ";

    line += client.getNickname().empty() ? "*" : client.getNickname();

    if (!params.empty())
        line += " " + params;

    line += text.empty() ? "" : " :" + text;

    sendReply(client, line);
}

std::string	CommandHandler::buildPrefix(const Client &client) const
{
    return ":" + client.getNickname() + "!"
               + client.getUsername() + "@"
               + (client.getHostname().empty() ? "localhost"
               : client.getHostname());
}

void CommandHandler::channelMessaging(Channel* channel,
        const std::string &message, Client* receiver)
{
    std::vector<Client*> members = channel->getMembers();
    for (std::vector<Client*>::iterator it
            = members.begin(); it != members.end(); ++it)
    {
        if (*it != receiver)
            sendReply(**it, message);
    }
}

void CommandHandler::channelMessaging(std::vector<Client*> members,
    const std::string &message, Client* receiver)
{
    for (std::vector<Client*>::iterator it
            = members.begin(); it != members.end(); ++it)
    {
        if (*it != receiver)
            sendReply(**it, message);
    }
}

void CommandHandler::handleTopic(Client& client, const Command& command)
{
    if (!client.isRegistered())
        return sendNumeric(client, "451", "", "You have not registered");

    if (command.getParameters().empty())
		return sendNumeric(client, "461", "TOPIC", "Not enough parameters");

    Channel *channel = _server.findChannel(command.getParameters()[0]);
    if (command.getParameters()[0].empty() 
		|| command.getParameters()[0][0] != '#'
        || channel == NULL)
        return sendNumeric(client, "403", command.getParameters()[0],
            "No such channel");

    if (command.getParameters().size() < 2)
    {
        if (channel->hasTopic())
			sendNumeric(client, "332", channel->getName(), channel->getTopic());
		else
			sendNumeric(client, "331", channel->getName(), "No topic is set");
    }
    else
    {
        std::string newTopic = command.getParameters()[1];
        ChannelResult result = channel->setTopic(&client, newTopic);
        if (result == SUCCESS)
        {
            std::string message = buildPrefix(client) + " TOPIC "
					+ channel->getName() + " :" + newTopic;
            channelMessaging(channel, message);
        }
        else
            handleChannelResult(client, result, channel->getName(), "");
    }
}

void CommandHandler::handleInvite(Client& client, const Command& command)
{
    if (!client.isRegistered())
        return sendNumeric(client, "451", "", "You have not registered");

    if (command.getParameters().size() < 2)
		return sendNumeric(client, "461", "INVITE", "Not enough parameters");

    Client *target = _server.findNick(command.getParameters()[0]);
    if (target == NULL)
        return sendNumeric(client, "401", command.getParameters()[0],
            "No such nickname/channel");
        
    Channel *channel = _server.findChannel(command.getParameters()[1]);
    if (channel == NULL)
        return sendNumeric(client, "403", command.getParameters()[1],
            "No such channel");

    ChannelResult result = channel->inviteMember(&client, target);
    if (result != SUCCESS)
        handleChannelResult(client, result, channel->getName(),
            target->getNickname());
    else
    {
        std::string message = buildPrefix(client) + " INVITE "
            + target->getNickname() + " " + channel->getName();
        sendReply(*target, message);
        sendNumeric(client, "341", target->getNickname() + " " +
            channel->getName(), "");
    }
}

void CommandHandler::handleKick(Client& client, const Command& command)
{
    if (!client.isRegistered())
        return sendNumeric(client, "451", "", "You have not registered");

    if (command.getParameters().size() < 2)
		return sendNumeric(client, "461", "KICK", "Not enough parameters");

    Channel *channel = _server.findChannel(command.getParameters()[0]);
    if (channel == NULL)
        return sendNumeric(client, "403", command.getParameters()[0],
            "No such channel");

    Client *target = _server.findNick(command.getParameters()[1]);
    if (target == NULL)
        return sendNumeric(client, "401", command.getParameters()[1],
            "No such nickname/channel");
    
    std::vector<Client*> members = channel->getMembers();
    ChannelResult result = channel->kickMember(&client, target);
    if (result != SUCCESS && result != CHANNEL_EMPTY)
        return handleChannelResult(client, result, channel->getName(),
            target->getNickname());

    std::string reason = command.getParameters().size() > 2
        ? command.getParameters()[2] : "No reason given";

    std::string message = buildPrefix(client) + " KICK "
            + channel->getName() + " " + target->getNickname()
            + " :" + reason;
    channelMessaging(members, message);
    //if (result == CHANNEL_EMPTY) <--- remove channel from server
}

void CommandHandler::handleMode(Client& client, const Command& command)
{
    (void)client;
    (void)command;
}