#include "../incl/CommandHandler.hpp"
#include "../incl/Server.hpp"
#include "../incl/Channel.hpp"

#include <cctype>

CommandHandler::CommandHandler(Server &server) : _server(server){}

CommandHandler::CommandHandler(const CommandHandler &other) : _server(other._server) {}

CommandHandler &CommandHandler::operator=(const CommandHandler &other) {
    (void)other;
    return *this;
}

CommandHandler::~CommandHandler() {}

// void CommandHandler::sendReply(Client &client, const std::string &message) {
//     client.appendToSendBuffer(message + "\r\n");
// }

void CommandHandler::sendReply(
    Client &client,
    const std::string &message)
{
    client.outbuf += message + "\r\n";
}

void CommandHandler::execute(
    Client &client,
    const Command &command
)
{
    const std::string &name = command.getName();

    if (name == "PASS")
        handlePass(client, command);
    else if (name == "NICK")
        handleNick(client, command);
    else if (name == "USER")
        handleUser(client, command);
    else if (name == "JOIN")
        handleJoin(client, command);
    else if (name == "PRIVMSG")
        handlePrivmsg(client, command);
    else
    {
        sendReply(
            client,
            ":ircserv 421 "
            + client.getNickname()
            + " "
            + name
            + " :Unknown command"
        );
    }
}


void CommandHandler::handlePass(
    Client &client,
    const Command &command
)
{
    if (client.isRegistered())
    {
        sendReply(
            client,
            ":ircserv 462 "
            + client.getNickname()
            + " :You may not reregister"
        );
        return;
    }

    if (command.getParameters().empty())
    {
        sendReply(
            client,
            ":ircserv 461 * PASS :Not enough parameters"
        );
        return;
    }

    if (command.getParameters()[0] != _server.getPassword())
    {
        sendReply(
            client,
            ":ircserv 464 * :Password incorrect"
        );
        return;
    }

    client.setPasswordAccepted(true);
    tryRegister(client);
}

bool CommandHandler::isValidNickname(
    const std::string &nickname
) const
{
    if (nickname.empty())
        return false;

    unsigned char first =
        static_cast<unsigned char>(nickname[0]);

    if (!std::isalpha(first)
        && nickname[0] != '_'
        && nickname[0] != '['
        && nickname[0] != ']'
        && nickname[0] != '\\'
        && nickname[0] != '`'
        && nickname[0] != '^'
        && nickname[0] != '{'
        && nickname[0] != '}'
        && nickname[0] != '|')
    {
        return false;
    }

    for (std::size_t i = 1; i < nickname.length(); ++i)
    {
        unsigned char c =
            static_cast<unsigned char>(nickname[i]);

        if (!std::isalnum(c)
            && nickname[i] != '-'
            && nickname[i] != '_'
            && nickname[i] != '['
            && nickname[i] != ']'
            && nickname[i] != '\\'
            && nickname[i] != '`'
            && nickname[i] != '^'
            && nickname[i] != '{'
            && nickname[i] != '}'
            && nickname[i] != '|')
        {
            return false;
        }
    }

    return true;
}

void CommandHandler::handleNick(
    Client &client,
    const Command &command
)
{
    if (command.getParameters().empty())
    {
        sendReply(
            client,
            ":ircserv 431 * :No nickname given"
        );
        return;
    }

    const std::string &newNickname =
        command.getParameters()[0];

    if (!isValidNickname(newNickname))
    {
        sendReply(
            client,
            ":ircserv 432 "
            + client.getNickname()
            + " "
            + newNickname
            + " :Erroneous nickname"
        );
        return;
    }

    Client *existing =
        _server.findNick(newNickname);

    if (existing != NULL && existing != &client)
    {
        sendReply(
            client,
            ":ircserv 433 "
            + client.getNickname()
            + " "
            + newNickname
            + " :Nickname is already in use"
        );
        return;
    }

    const std::string oldNickname = client.getNickname();

    client.setNickname(newNickname);

    if (client.isRegistered())
    {
        const std::string nickMessage =
            ":" + oldNickname
            + "!" + client.getUsername()
            + "@" + client.getHostname()
            + " NICK :" + newNickname;

        // _server.broadcastToClientChannels(
        //     client,
        //     nickMessage
        // );

        sendReply(client, nickMessage);
    }

    tryRegister(client);
}

void CommandHandler::handleUser(
    Client &client,
    const Command &command
)
{
    if (client.isRegistered())
    {
        sendReply(
            client,
            ":ircserv 462 "
            + client.getNickname()
            + " :You may not reregister"
        );
        return;
    }

    if (command.getParameters().size() < 4)
    {
        sendReply(
            client,
            ":ircserv 461 "
            + client.getNickname()
            + " USER :Not enough parameters"
        );
        return;
    }

    client.setUsername(command.getParameters()[0]);
    client.setRealname(command.getParameters()[3]);
    tryRegister(client);
}

// void CommandHandler::tryRegister(Client &client)
// {
//     if (client.isRegistered())
//         return;

//     if (!client.isPasswordAccepted())
//         return;

//     if (client.getNickname().empty())
//         return;

//     if (!client.isUserReceived())
//         return;

//     client.setRegistered(true);

//     sendReply(
//         client,
//         ":ircserv 001 "
//         + client.getNickname()
//         + " :Welcome to the IRC server "
//         + client.getNickname()
//     );

//     sendReply(
//         client,
//         ":ircserv 002 "
//         + client.getNickname()
//         + " :Your host is ircserv"
//     );

//     sendReply(
//         client,
//         ":ircserv 003 "
//         + client.getNickname()
//         + " :This server was created for ft_irc"
//     );

//     sendReply(
//         client,
//         ":ircserv 004 "
//         + client.getNickname()
//         + " ircserv 1.0 itkol"
//     );
// }

void CommandHandler::tryRegister(Client &client)
{
    if (client.isRegistered())
        return;

    if (!client.isPasswordAccepted())
        return;

    if (client.getNickname().empty())
        return;

    if (client.getUsername().empty())
        return;

    client.setRegistered(true);

    sendReply(
        client,
        ":ircserv 001 "
        + client.getNickname()
        + " :Welcome to ircserv "
        + client.getNickname()
    );
}

// void CommandHandler::handleJoin(
//     Client &client,
//     const Command &command
// )
// {
//     if (!client.isRegistered())
//     {
//         sendReply(
//             client,
//             ":ircserv 451 * :You have not registered"
//         );
//         return;
//     }

//     if (command.getParameters().empty())
//     {
//         sendReply(
//             client,
//             ":ircserv 461 "
//             + client.getNickname()
//             + " JOIN :Not enough parameters"
//         );
//         return;
//     }

//     const std::string &channelName =
//         command.getParameters()[0];

//     if (channelName.empty() || channelName[0] != '#')
//     {
//         sendReply(
//             client,
//             ":ircserv 403 "
//             + client.getNickname()
//             + " "
//             + channelName
//             + " :No such channel"
//         );
//         return;
//     }

//     std::string key;

//     if (command.getParameters().size() > 1)
//         key = command.getParameters()[1];

//     Channel *channel =
//         _server.findChannel(channelName);

//     if (channel == NULL)
//     {
//         channel = &_server.createChannel(channelName);

//         channel->addClient(client);
//         channel->addOperator(client);
//     }
//     else
//     {
//         if (channel->hasClient(client))
//             return;

//         if (channel->isInviteOnly()
//             && !channel->isInvited(client))
//         {
//             sendReply(
//                 client,
//                 ":ircserv 473 "
//                 + client.getNickname()
//                 + " "
//                 + channelName
//                 + " :Cannot join channel (+i)"
//             );
//             return;
//         }

//         if (channel->hasKey()
//             && channel->getKey() != key)
//         {
//             sendReply(
//                 client,
//                 ":ircserv 475 "
//                 + client.getNickname()
//                 + " "
//                 + channelName
//                 + " :Cannot join channel (+k)"
//             );
//             return;
//         }

//         if (channel->hasUserLimit()
//             && channel->getClientCount()
//                 >= channel->getUserLimit())
//         {
//             sendReply(
//                 client,
//                 ":ircserv 471 "
//                 + client.getNickname()
//                 + " "
//                 + channelName
//                 + " :Cannot join channel (+l)"
//             );
//             return;
//         }

//         channel->addClient(client);
//         channel->removeInvite(client);
//     }

//     const std::string joinMessage =
//         ":" + client.getNickname()
//         + "!" + client.getUsername()
//         + "@" + client.getHostname()
//         + " JOIN :" + channelName;

//     channel->broadcast(joinMessage, NULL);

//     if (!channel->getTopic().empty())
//     {
//         sendReply(
//             client,
//             ":ircserv 332 "
//             + client.getNickname()
//             + " "
//             + channelName
//             + " :" + channel->getTopic()
//         );
//     }
//     else
//     {
//         sendReply(
//             client,
//             ":ircserv 331 "
//             + client.getNickname()
//             + " "
//             + channelName
//             + " :No topic is set"
//         );
//     }

//     sendReply(
//         client,
//         ":ircserv 353 "
//         + client.getNickname()
//         + " = "
//         + channelName
//         + " :" + channel->getNamesList()
//     );

//     sendReply(
//         client,
//         ":ircserv 366 "
//         + client.getNickname()
//         + " "
//         + channelName
//         + " :End of NAMES list"
//     );
// }

// void CommandHandler::handlePrivmsg(
//     Client &client,
//     const Command &command
// )
// {
//     if (!client.isRegistered())
//     {
//         sendReply(
//             client,
//             ":ircserv 451 * :You have not registered"
//         );
//         return;
//     }

//     if (command.getParameters().empty())
//     {
//         sendReply(
//             client,
//             ":ircserv 411 "
//             + client.getNickname()
//             + " :No recipient given (PRIVMSG)"
//         );
//         return;
//     }

//     if (command.getParameters().size() < 2
//         || command.getParameters()[1].empty())
//     {
//         sendReply(
//             client,
//             ":ircserv 412 "
//             + client.getNickname()
//             + " :No text to send"
//         );
//         return;
//     }

//     const std::string &target =
//         command.getParameters()[0];

//     const std::string &text =
//         command.getParameters()[1];

//     const std::string message =
//         ":" + client.getNickname()
//         + "!" + client.getUsername()
//         + "@" + client.getHostname()
//         + " PRIVMSG "
//         + target
//         + " :" + text;

//     if (!target.empty() && target[0] == '#')
//     {
//         Channel *channel =
//             _server.findChannel(target);

//         if (channel == NULL)
//         {
//             sendReply(
//                 client,
//                 ":ircserv 403 "
//                 + client.getNickname()
//                 + " "
//                 + target
//                 + " :No such channel"
//             );
//             return;
//         }

//         if (!channel->hasClient(client))
//         {
//             sendReply(
//                 client,
//                 ":ircserv 404 "
//                 + client.getNickname()
//                 + " "
//                 + target
//                 + " :Cannot send to channel"
//             );
//             return;
//         }

//         channel->broadcast(message, &client);
//         return;
//     }

//     Client *targetClient =
//         _server.findClientByNickname(target);

//     if (targetClient == NULL)
//     {
//         sendReply(
//             client,
//             ":ircserv 401 "
//             + client.getNickname()
//             + " "
//             + target
//             + " :No such nick"
//         );
//         return;
//     }

//     sendReply(*targetClient, message);
// }

void CommandHandler::handleJoin(
    Client &client,
    const Command &command)
{
    (void)client;
    (void)command;
}

void CommandHandler::handlePrivmsg(
    Client &client,
    const Command &command)
{
    (void)client;
    (void)command;
}