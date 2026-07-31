#pragma once 

#include "../incl/Command.hpp"
#include "../incl/Client.hpp"

class CommandHandler{
public:
	CommandHandler(){};
	CommandHandler(CommandHandler &other){};
	CommandHandler &operator=(CommandHandler &other){};
	~CommandHandler(){};

    void handlePass(Client &client, const Command &command);
    void handleNick(Client &client, const Command &command);
    void handleUser(Client &client, const Command &command);
    void handleJoin(Client &client, const Command &command);
    void handlePrivmsg(Client &client, const Command &command);
}