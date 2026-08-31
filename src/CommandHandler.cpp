#include "../incl/CommandHandler.hpp"
#include "../incl/Server.hpp"

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
    else if (cmd == "MODE")
        handleMode(client, command);
	else if (cmd == "PART")
       handlePart(client, command);
    else if (cmd == "QUIT")
        handleQuit(client, command);
    else if (cmd == "PING")
        handlePing(client, command);
	else if (cmd == "CAP")
        handleCap(client, command);
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

// ============================================================================
//                                HANDLE PASS
// ============================================================================

void CommandHandler::handlePass(Client &client, const Command &command) {
    if (client.isRegistered()) {
        sendReply(
            client,
            ":ircserv 462 "
            + client.getNickname()
            + " :You are alredy registered"
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
        client.setPasswordAccepted(false);
        sendReply(
            client,
            ":ircserv 464 * :Password incorrect"
        );
        return;
    }

    client.setPasswordAccepted(true);
	std::cout << "Password accepted for client" << std::endl;
    tryRegister(client);
}

// ============================================================================
//                              HANDLE NICK
// ============================================================================

void CommandHandler::handleNick(Client &client, const Command &command) {
    if (command.getParameters().empty()) {
        sendReply(
            client,
            ":ircserv 431 * :No nickname given"
        );
        return;
    }
    std::string newNickname = command.getParameters()[0];
    const std::size_t NICKLEN = 30;
    if (newNickname.size() > NICKLEN)
        newNickname = newNickname.substr(0, NICKLEN);

    if (client.isRegistered()) {
        const std::string message =
            buildPrefix(client)
            + " NICK :"
            + newNickname;

        _server.broadcastNickChange(client, message);
        client.setNickname(newNickname);
        return;
    }

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
    client.setNickname(newNickname);
	std::cout << "Nickname accepted for client" << std::endl;
    tryRegister(client);
}

// ============================================================================
//                                 HANDLE USER
// ============================================================================

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
            + " USER :Usage: USER <username> 0 * :<realname>"
        );
        return;
    }

    client.setUsername(command.getParameters()[0]);
    client.setRealname(command.getParameters()[3]);
	client.setUserReceived(true);
	std::cout << "User accepted for client" << std::endl;
    tryRegister(client);
}

// ============================================================================
//                               REGISTRATION
// ============================================================================

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

// ============================================================================
//                               HANDLE CAPABILITIES
// ============================================================================

void CommandHandler::handleCap(Client &client, const Command &command)
{
    if (command.getParameters().empty())
        return;

    const std::string sub = command.getParameters()[0];

    if (sub == "LS") {
        sendReply(client, ":ircserv CAP * LS :");
    }
    else if (sub == "REQ") {
        std::string requested;

        if (command.getParameters().size() > 1)
            requested = command.getParameters()[1];

        sendReply(
            client,
            ":ircserv CAP * NAK :" + requested
        );
    }
    else if (sub == "END")
        return;
}

// ============================================================================
//                               HANDLE CHANNEL RESULTS
// ============================================================================

void CommandHandler::handleChannelResult(Client &client, ChannelResult result, const std::string &channelName, const std::string &target) {
    const std::string nick =
        client.getNickname().empty()
        ? "*"
        : client.getNickname();

    if (result == SUCCESS || result == NO_CHANGE) {
        return;
    }

    if (result == CHANNEL_EMPTY) {
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

// ============================================================================
//                           HANDLE JOIN
// ============================================================================
void CommandHandler::handleJoinZero(Client &client) {
    std::map<std::string, Channel> &channels = _server.getChannels();
    std::map<std::string, Channel>::iterator it = channels.begin();

    while (it != channels.end()) {
        Channel &channel = it->second;

        if (!channel.isMember(&client)) {
            ++it;
            continue;
        }

        const std::string message =
            buildPrefix(client)
            + " PART "
            + it->first
            + " :Left all channels";

        channelMessaging(
            &channel,
            message
        );

        ChannelResult result = channel.removeMember(&client);

        if (result == CHANNEL_EMPTY) {
            std::map<std::string, Channel>::iterator toDelete = it;
            ++it;
            channels.erase(toDelete);
        } else {
            ++it;
        }
    }
}

void CommandHandler::handleJoin(Client &client, const Command &command) {
    if (!client.isRegistered())     {
        sendReply(
            client,
            ":ircserv 451 * :You have registered"
        );
        return;
    }

    if (command.getParameters().empty()) {
        sendReply(
            client,
            ":ircserv 461 "
            + client.getNickname()
            + " JOIN :Not enough parameters"
        );
        return;
    }

    const std::string &channelName = command.getParameters()[0];
    if (channelName == "0") {
        handleJoinZero(client);
        return;
    }

    if (channelName.empty() || channelName[0] != '#') {
        sendReply(
            client,
            ":ircserv 476 "
            + client.getNickname()
            + " "
            + channelName
            + " :Invalid channel name"
        );
        return;
    }

    std::string key;

    if (command.getParameters().size() > 1)
        key = command.getParameters()[1];

    Channel *channel = _server.findChannel(channelName);

    if (channel == NULL) {
        channel = &_server.createChannel(channelName);
    }

    ChannelResult result = channel->addMember(&client, key);

    if (result == SUCCESS)
    {
        const std::string message =
            buildPrefix(client)
            + " JOIN :"
            + channelName;


        channelMessaging(channel, message);

		std::string names;

		const std::vector<Client *> &members = channel->getMembers();

		for (
			std::vector<Client *>::const_iterator it = members.begin();
			it != members.end();
			++it
		)
		{
			if (!names.empty())
				names += " ";

			names += (*it)->getNickname();
		}

		sendReply(
			client,
			":ircserv 353 "
			+ client.getNickname()
			+ " = "
			+ channelName
			+ " :"
			+ names
		);

		sendReply(
			client,
			":ircserv 366 "
			+ client.getNickname()
			+ " "
			+ channelName
			+ " :End of /NAMES list"
		);


        return;
    }



    if (result == ALREADY_MEMBER)
        return;

    handleChannelResult(
        client,
        result,
        channelName,
        ""
    );
}

// ============================================================================
//                         HANDLE PRIVMSG
// ============================================================================

void CommandHandler::handlePrivmsg(Client &client, const Command &command) {
    if (!client.isRegistered()) {
        sendReply(
            client,
            ":ircserv 451 * :You have not registered"
        );
        return;
    }

    if (command.getParameters().empty()) {
        sendReply(
            client,
            ":ircserv 411 "
            + client.getNickname()
            + " :No recipient given (PRIVMSG)"
        );
        return;
    }

    if (command.getParameters().size() < 2 || command.getParameters()[1].empty()) {
        sendReply(
            client,
            ":ircserv 412 "
            + client.getNickname()
            + " :No text to send"
        );
        return;
    }

    const std::string &target = command.getParameters()[0];
    const std::string &text = command.getParameters()[1];

    const std::string message =
        ":" + client.getNickname()
        + "!" + client.getUsername()
        + "@" + client.getHostname()
        + " PRIVMSG "
        + target
        + " :" + text;

    if (!target.empty() && target[0] == '#') {
        Channel *channel = _server.findChannel(target);

        if (channel == NULL) {
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

        if (!channel->isMember(&client)) {
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
        channelMessaging(
            channel,
            message,
            &client
        );
        return;
    }

    Client *targetClient = _server.findNick(target);

    if (targetClient == NULL) {
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

// ============================================================================
//                         HANDLE PART
// ============================================================================

void CommandHandler::handlePart(Client &client, const Command &command) {
    if (command.getParameters().empty()) {
        sendReply(
            client,
            ":ircserv 461 "
            + client.getNickname()
            + " PART :Not enough parameters"
        );
        return;
    }

    if (!client.isRegistered()) {
        sendReply(
            client,
            ":ircserv 451 * :You have not registered"
        );
        return;
    }

    const std::string channelName = command.getParameters()[0];
    Channel *channel = _server.findChannel(channelName);

    if (channel == NULL) {
        sendReply(
            client,
            ":ircserv 403 "
            + client.getNickname()
            + " "
            + channelName
            + " :No such channel"
        );
        return;
    }

    if (!channel->isMember(&client)) {
        sendReply(
            client,
            ":ircserv 442 "
            + client.getNickname()
            + " "
            + channelName
            + " :You're not on that channel"
        );
        return;
    }

    std::string message =
        buildPrefix(client)
        + " PART "
        + channelName;

    if (command.getParameters().size() > 1) {
        message += " :"
            + command.getParameters()[1];
    }

    channelMessaging(
        channel,
        message
    );

    ChannelResult result = channel->removeMember(&client);

    if (result == CHANNEL_EMPTY)
        _server.removeChannel(channelName);
}

// ============================================================================
//                         HANDLE QUIT
// ============================================================================

//TODO: nc: write failed (0/2): Broken pipe
void CommandHandler::handleQuit(Client &client, const Command &command) {
    std::string reason = "Leaving";

    if (!command.getParameters().empty())
        reason = command.getParameters()[0];

    std::string message =
        ":" + client.getNickname()
        + "!" + client.getUsername()
        + "@localhost QUIT :"
        + reason;

    _server.broadcastQuit(client, message);
    _server.disconnectClient(client);
}


// ============================================================================
//                         HANDLE PING
// ============================================================================

void CommandHandler::handlePing(Client &client, const Command &command) {
    if (command.getParameters().empty()) {
        sendReply(
            client,
            ":ircserv 409 "
            + client.getNickname()
            + " :No origin specified"
        );
        return;
    }

    sendReply(
        client,
        ":ircserv PONG ircserv :"
        + command.getParameters()[0]
    );
}
