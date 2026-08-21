#pragma once 

#include "Command.hpp"
#include "Client.hpp"
#include "Channel.hpp"

class Server;

class CommandHandler
{
private:
    Server &_server;
    void sendReply(Client &client, const std::string &message);
    void tryRegister(Client &client);
    bool isValidNickname(const std::string &nickname) const;
	CommandHandler(const CommandHandler &other);
	CommandHandler &operator=(const CommandHandler &other);
	std::string	buildPrefix(const Client &) const;
	void handlePass(Client &client, const Command &command);
	void handleNick(Client &client, const Command &command);
	void handleUser(Client &client, const Command &command);
	void handleJoin(Client &client, const Command &command);
	void handlePrivmsg(Client &client, const Command &command);
	void handleChannelResult(Client &client, ChannelResult result,
		const std::string &channelName, const std::string &target);

	void channelMessaging(Channel*, const std::string &,
		Client* receiver = NULL);
	void channelMessaging(std::vector<Client*>,
		const std::string &, Client* receiver = NULL);
	void handleTopic(Client&, const Command&);
	void handleInvite(Client&, const Command&);
	void handleKick(Client&, const Command&);
	void handleMode(Client&, const Command&);

	void sendNumeric(Client &client, const std::string &code,
                 const std::string &params, const std::string &text);
	
public:
    CommandHandler(Server &server);
    ~CommandHandler();
	
	void execute(Client &client, const Command &command);
	// std::map<std::string, CommandType> commands;
};

