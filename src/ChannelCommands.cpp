#include "../incl/CommandHandler.hpp"
#include "../incl/Server.hpp"
#include "../incl/CommandUtils.hpp"

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

void CommandHandler::channelMessaging(const std::vector<Client*>& members,
    const std::string &message, Client* receiver)
{
    for (std::vector<Client*>::const_iterator it
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
    if (result == CHANNEL_EMPTY)
    {
        _server.removeChannel(channel->getName());
    }
}

void CommandHandler::handleMode(Client& client, const Command& command)
{
    if (!client.isRegistered())
        return sendNumeric(client, "451", "", "You have not registered");

    if (command.getParameters().empty()
        || command.getParameters()[0].empty())
        return sendNumeric(client, "461", "MODE", "Not enough parameters");
    else if (command.getParameters()[0][0] == '#')
    {
        Channel *channel = _server.findChannel(command.getParameters()[0]);
        if (channel == NULL)
            return sendNumeric(client, "403", command.getParameters()[0],
                "No such channel");
        if (command.getParameters().size() == 1)
        {
            std::string mode = channel->getMode(&client);
            sendNumeric(client, "324", channel->getName(), mode);
        }
        else
        {
            if (!channel->isMember(&client))
                return sendNumeric(client, "442", channel->getName(),
                    "You're not on that channel");
            else if (!channel->isOperator(&client))
                return sendNumeric(client, "482", channel->getName(),
                    "You're not channel operator");

            ChannelResult result;
            std::string successedModes = "";
            std::string successedParams = "";
            std::string modeChange = command.getParameters()[1];
            if (modeChange.empty())
                return sendNumeric(client, "461", "MODE", "Not enough parameters");
            bool settingMode = (modeChange[0] == '+');
            size_t argIndex = 2;
            for (size_t i = 1; i < modeChange.size(); ++i)
            {
                std::string argument = argIndex < command.getParameters().size()
                    ? command.getParameters()[argIndex] : "";
                if (modeChange[i] == '+' || modeChange[i] == '-')
                {
                    settingMode = (modeChange[i] == '+') ? true : false;
                    continue;
                }
                if (modeChange[i] == 'o') // operator
                {
                    if (argIndex >= command.getParameters().size())
                    {
                        sendNumeric(client, "461", "MODE",
                            "Not enough parameters");
                        continue;
                    }
                    argIndex++;
                    Client *target = _server.findNick(argument);
                    if (target == NULL)
                    {
                        sendNumeric(client, "401", argument,
                            "No such nickname/channel");
                        continue;
                    }
                    result = channel->setOperator(&client, target, settingMode);
                    successedModes += result == SUCCESS ? (settingMode ? "+o" : "-o") : "";
                    successedParams += result == SUCCESS ? " " + target->getNickname() : "";
                    handleChannelResult(client, result, channel->getName(),
                        target->getNickname());
                }
                else if (modeChange[i] == 'i') // invite only
                {
                    result = channel->setInviteOnly(&client, settingMode);
                    successedModes += result == SUCCESS ? (settingMode ? "+i" : "-i") : "";
                    handleChannelResult(client, result, channel->getName(), "");
                }
                else if (modeChange[i] == 't') // protected topic
                {
                    result = channel->setProtectedTopic(&client, settingMode);
                    successedModes += result == SUCCESS ? (settingMode ? "+t" : "-t") : "";
                    handleChannelResult(client, result, channel->getName(), "");
                }
                else if (modeChange[i] == 'l') // user limit
                {
                    if (!settingMode)
                    {
                        result = channel->setUserLimit(&client, 0);
                        successedModes += result == SUCCESS ? "-l" : "";
                        handleChannelResult(client, result, channel->getName(), "");
                        continue;
                    }
                    if (argIndex >= command.getParameters().size())
                    {
                        sendNumeric(client, "461", "MODE", "Not enough parameters");
                        continue;
                    }
                    argIndex++;
                    if (argument.find_first_not_of("0123456789") != std::string::npos)
                    {
                        sendNumeric(client, "461", "MODE", "Invalid parameter");
                        continue;
                    }
                    size_t limit = atol(argument.c_str());
                    result = channel->setUserLimit(&client, limit);
                    successedModes += result == SUCCESS ? "+l" : "";
                    successedParams += result == SUCCESS ? " " + argument : "";
                    handleChannelResult(client, result, channel->getName(), "");
                }
                else if (modeChange[i] == 'k') // key
                {
                    if (settingMode && argIndex >= command.getParameters().size())
                    {
                        sendNumeric(client, "461", "MODE", "Not enough parameters");
                        continue;
                    }
                    if (settingMode)
                        argIndex++;
                    std::string key = settingMode ? argument : "";
                    result = channel->setKey(&client, key);
                    successedModes += result == SUCCESS ? (settingMode ? "+k" : "-k") : "";
                    successedParams += result == SUCCESS && settingMode ? " " + key : "";
                    handleChannelResult(client, result, channel->getName(), "");
                }
                else
                {
                    sendNumeric(client, "472", std::string(1, modeChange[i]),
                        "Unknown MODE flag");
                }
            }
            if (!successedModes.empty())
            {
                char modeChar = successedModes[0];
                for (size_t j = 1; j < successedModes.length(); j++)
                {
                    if (successedModes[j] == '+' || successedModes[j] == '-')
                    {
                        if (successedModes[j] != modeChar)
                            modeChar = successedModes[j];
                        else
                        {
                            successedModes.erase(j, 1);
                            j--;
                        }
                    }
                }
                std::string channelMsg = buildPrefix(client) + " MODE " + channel->getName() + " " + successedModes;
                if (!successedParams.empty())
                    channelMsg += successedParams;
                channelMessaging(channel, channelMsg);
            }
        }
    }
    else
    {
        sendNumeric(client, "501", "", "Unknown MODE flag");
    }
}