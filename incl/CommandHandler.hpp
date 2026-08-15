#pragma once 

#include "../incl/Command.hpp"
#include "../incl/Client.hpp"

class Server;

class CommandHandler
{
private:
    Server &_server;

    void sendReply(Client &client, const std::string &message);
    void tryRegister(Client &client);
    bool isValidNickname(const std::string &nickname) const;

public:
    CommandHandler(Server &server);
    CommandHandler(const CommandHandler &other);
    CommandHandler &operator=(const CommandHandler &other);
    ~CommandHandler();

    void execute(Client &client, const Command &command);

    void handlePass(Client &client, const Command &command);
    void handleNick(Client &client, const Command &command);
    void handleUser(Client &client, const Command &command);

    // Поки залишимо оголошення, але реалізацію нижче тимчасово спростимо
    void handleJoin(Client &client, const Command &command);
    void handlePrivmsg(Client &client, const Command &command);
};

