#include "../incl/CommandHandler.hpp"
#include "../incl/Server.hpp"
#include "../incl/Utils.hpp"


CommandHandler::CommandHandler(Server &server) : _server(server) {}

CommandHandler::~CommandHandler() {}

void CommandHandler::sendReply(Client &client, const std::string &message) {
    _server.queue(client, message + "\r\n");
}

void CommandHandler::execute(Client &client, const Command &command) {
    const std::string &cmd = command.getName();

    if (cmd == "PASS")
        handlePass(client, command);
    else if (cmd == "NICK")
        handleNick(client, command);
    else if (cmd == "USER")
        handleUser(client, command);
    else if (cmd == "JOIN")
        handleJoin(client, command);
    else if (cmd == "PRIVMSG")
        handlePrivmsg(client, command);
    else if (cmd == "TOPIC")
        handleTopic(client, command);
    else if (cmd == "INVITE")
        handleInvite(client, command);
    else if (cmd == "KICK")
        handleKick(client, command);
    else
    {
        sendReply(
            client,
            ":ircserv 421 "
            + client.getNickname()
            + " "
            + cmd
            + " :Unknown command"
        );
    }
}

void CommandHandler::handlePass(Client &client, const Command &command) {
    if (client.isRegistered())     {
        sendReply(
            client,
            ":ircserv 462 "
            + client.getNickname()
            + " :You may not reregister"
        );
        return;
    }

    if (command.getParameters().empty()) {
        sendReply(
            client,
            ":ircserv 461 * PASS :Not enough parameters"
        );
        return;
    }

    if (command.getParameters()[0] != _server.getPassword()) {
        sendReply(
            client,
            ":ircserv 464 * :Password incorrect"
        );
        return;
    }

    client.setPasswordAccepted(true);
    tryRegister(client);
}

bool CommandHandler::isValidNickname(const std::string &nickname) const {
    if (nickname.empty())
        return false;

    if (!std::isalpha(
            static_cast<unsigned char>(nickname[0]))
        && !isNickSpecial(nickname[0])) {
        return false;
    }

    for (std::size_t i = 1; i < nickname.size(); ++i) {
        if (!std::isalnum(
                static_cast<unsigned char>(nickname[i]))
            && nickname[i] != '-'
            && !isNickSpecial(nickname[i])) {
            return false;
        }
    }
    return true;
}

void CommandHandler::handleNick(Client &client, const Command &command) {
    if (command.getParameters().empty()) {
        sendReply(
            client,
            ":ircserv 431 * :No nickname given"
        );
        return;
    }

    const std::string &newNickname = command.getParameters()[0];

    if (!isValidNickname(newNickname)) {
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

    Client *existing = _server.findNick(newNickname);

    if (existing != NULL && existing != &client) {
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

    /*
     * ВАЖЛИВО:
     * nickname треба реально встановити.
     */
    client.setNickname(newNickname);

    /*
     * Якщо client вже registered, пізніше тут
     * треба буде broadcast NICK change по каналах.
     */

    tryRegister(client);
}

void CommandHandler::handleUser(Client &client, const Command &command) {
    if (client.isRegistered())     {
        sendReply(
            client,
            ":ircserv 462 "
            + client.getNickname()
            + " :You may not reregister"
        );
        return;
    }

    if (command.getParameters().size() < 4) {
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
	client.setUserReceived(true);
    tryRegister(client);
}

void CommandHandler::tryRegister(Client &client) {
    if (client.isRegistered())
        return;
    if (!client.isPasswordAccepted())
        return;
    if (client.getNickname().empty())
        return;
    if (!client.isUserReceived())
        return;
    client.setRegistered(true);
    sendReply(
        client,
        ":ircserv 001 "
        + client.getNickname()
        + " :Welcome to the IRC server "
        + client.getNickname()
    );
    sendReply(
        client,
        ":ircserv 002 "
        + client.getNickname()
        + " :Your host is ircserv"
    );
    sendReply(
        client,
        ":ircserv 003 "
        + client.getNickname()
        + " :This server was created for ft_irc"
    );
    sendReply(
        client,
        ":ircserv 004 "
        + client.getNickname()
        + " ircserv 1.0 itkol"
    );
}

void CommandHandler::handleChannelResult(Client &client, ChannelResult result, const std::string &channelName, const std::string &target) {
    const std::string nick =
        client.getNickname().empty()
        ? "*"
        : client.getNickname();

    if (result == SUCCESS || result == NO_CHANGE) {
        return;
    }

    if (result == CHANNEL_EMPTY) {
        /*
         * Це НЕ IRC error.
         * Пізніше Server може видалити channel.
         */
        return;
    }

    if (result == NO_TOPIC) {
        sendReply(
            client,
            ":ircserv 331 "
            + nick
            + " "
            + channelName
            + " :No topic is set"
        );
    }
    else if (result == NOT_IN_CHANNEL)
    {
        sendReply(
            client,
            ":ircserv 441 "
            + nick
            + " "
            + target
            + " "
            + channelName
            + " :They aren't on that channel"
        );
    }
    else if (result == NOT_ON_CHANNEL)
    {
        sendReply(
            client,
            ":ircserv 442 "
            + nick
            + " "
            + channelName
            + " :You're not on that channel"
        );
    }
    else if (result == ALREADY_MEMBER)
    {
        sendReply(
            client,
            ":ircserv 443 "
            + nick
            + " "
            + target
            + " "
            + channelName
            + " :is already on channel"
        );
    }
    else if (result == NOT_OPERATOR)
    {
        sendReply(
            client,
            ":ircserv 482 "
            + nick
            + " "
            + channelName
            + " :You're not channel operator"
        );
    }
    else if (result == CHANNEL_FULL)
    {
        sendReply(
            client,
            ":ircserv 471 "
            + nick
            + " "
            + channelName
            + " :Cannot join channel (+l)"
        );
    }
    else if (result == INVITE_ONLY)
    {
        sendReply(
            client,
            ":ircserv 473 "
            + nick
            + " "
            + channelName
            + " :Cannot join channel (+i)"
        );
    }
    else if (result == BAD_KEY)
    {
        sendReply(
            client,
            ":ircserv 475 "
            + nick
            + " "
            + channelName
            + " :Cannot join channel (+k)"
        );
    }
}

void CommandHandler::handleJoin(
    Client &client,
    const Command &command)
{
    if (!client.isRegistered())
    {
        sendReply(
            client,
            ":ircserv 451 * :You have not registered"
        );
        return;
    }

    if (command.getParameters().empty())
    {
        sendReply(
            client,
            ":ircserv 461 "
            + client.getNickname()
            + " JOIN :Not enough parameters"
        );
        return;
    }

    const std::string &channelName =
        command.getParameters()[0];

    std::string key;

    if (command.getParameters().size() > 1)
        key = command.getParameters()[1];

    Channel *channel =
        _server.findChannel(channelName);

    /*
     * Якщо Server поки НЕ готовий створювати Channel,
     * цей шматок треба тимчасово закоментувати.
     */
    if (channel == NULL)
    {
        channel =
            &_server.createChannel(channelName);
    }

    ChannelResult result =
        channel->addMember(
            &client,
            key
        );

    if (result == SUCCESS)
    {
        /*
         * Person 3 вже додав member.
         *
         * TODO:
         * - broadcast JOIN
         * - 331/332 topic
         * - 353 names
         * - 366 end of names
         *
         * Для цього потрібен додатковий API.
         */
        return;
    }

    /*
     * Уже member — для JOIN можна просто нічого не робити.
     */
    if (result == ALREADY_MEMBER)
        return;

    handleChannelResult(
        client,
        result,
        channelName,
        ""
    );
}

void CommandHandler::handlePrivmsg(
    Client &client,
    const Command &command)
{
    if (!client.isRegistered())
    {
        sendReply(
            client,
            ":ircserv 451 * :You have not registered"
        );
        return;
    }

    if (command.getParameters().empty())
    {
        sendReply(
            client,
            ":ircserv 411 "
            + client.getNickname()
            + " :No recipient given (PRIVMSG)"
        );
        return;
    }

    if (command.getParameters().size() < 2
        || command.getParameters()[1].empty())
    {
        sendReply(
            client,
            ":ircserv 412 "
            + client.getNickname()
            + " :No text to send"
        );
        return;
    }

    const std::string &target =
        command.getParameters()[0];

    const std::string &text =
        command.getParameters()[1];

    /*
     * Поки без hostname, бо Client API його не дає.
     */
    const std::string message =
        ":" + client.getNickname()
        + "!" + client.getUsername()
        + " PRIVMSG "
        + target
        + " :" + text;

    /*
     * CHANNEL PRIVMSG
     */
    if (!target.empty() && target[0] == '#')
    {
        Channel *channel =
            _server.findChannel(target);

        if (channel == NULL)
        {
            sendReply(
                client,
                ":ircserv 403 "
                + client.getNickname()
                + " "
                + target
                + " :No such channel"
            );
            return;
        }

        if (!channel->isMember(&client))
        {
            sendReply(
                client,
                ":ircserv 404 "
                + client.getNickname()
                + " "
                + target
                + " :Cannot send to channel"
            );
            return;
        }

        /*
         * У Channel Person 3 немає broadcast().
         *
         * TODO:
         * Person 1 / Server повинен надати щось типу:
         *
         * _server.broadcastChannel(
         *     channel,
         *     message,
         *     &client
         * );
         */

        return;
    }

    /*
     * USER PRIVMSG
     */
    Client *targetClient =
        _server.findNick(target);

    if (targetClient == NULL)
    {
        sendReply(
            client,
            ":ircserv 401 "
            + client.getNickname()
            + " "
            + target
            + " :No such nick"
        );
        return;
    }

    sendReply(
        *targetClient,
        message
    );
}
