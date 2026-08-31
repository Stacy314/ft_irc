#pragma once 

#include "Command.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "CommandUtils.hpp"

class Server;

class CommandHandler
{
private:
	CommandHandler(const CommandHandler &other);
	CommandHandler &operator=(const CommandHandler &other);
	
    Server &_server;
    void sendReply(Client &client, const std::string &message);
    void tryRegister(Client &client);
	void handlePass(Client &client, const Command &command);
	void handleNick(Client &client, const Command &command);
	void handleUser(Client &client, const Command &command);
	void handleJoin(Client &client, const Command &command);
	void handlePrivmsg(Client &client, const Command &command);
	void handleTopic(Client&, const Command&);
	void handleInvite(Client&, const Command&);
	void handleKick(Client&, const Command&);
	void handleMode(Client&, const Command&);
	void handlePart(Client &client, const Command &command);
	void handleQuit(Client &client, const Command &command);
	void handlePing(Client &client, const Command &command);
	void handleCap(Client &client, const Command &command);
	void handleChannelResult(Client &client, ChannelResult result,
		const std::string &channelName, const std::string &target);
	void channelMessaging(Channel*, const std::string &,
		Client* receiver = NULL);
	void channelMessaging(const std::vector<Client*>&,
		const std::string &, Client* receiver = NULL);
	void sendNumeric(Client &client, const std::string &code,
                 const std::string &params, const std::string &text);
	void handleJoinZero(Client &client);
public:
    CommandHandler(Server &server);
    ~CommandHandler();
	
	void execute(Client &client, const Command &command);
	std::string buildPrefix(const Client &client) const;
};
