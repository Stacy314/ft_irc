#include "../incl/CommandHandler.hpp"
#include "../incl/Server.hpp"
#include "../incl/Utils.hpp"

void CommandHandler::sendNumeric(Client &client, const std::string &code,
                 const std::string &params, const std::string &text)
{
	std::string ircserv = "ircserv";
    std::string line = ":" + /*_server.serverName()*/ircserv + " " + code + " ";

    line += client.getNickname().empty() ? "*" : client.getNickname();

    if (!params.empty())
        line += " " + params;

    line += " :" + text;

    sendReply(client, line);
}

std::string	CommandHandler::buildPrefix(const Client &client) const
{
    return ":" + client.getNickname() + "!"
                + client.getUsername() + "@" + "localhost";
    // change "localhost" to client.getHost() when Client
    // class has getHost() method
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
        return sendNumeric(client, "403", command.getParameters()[0], "No such channel");

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