#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"

#include <map>
#include <string>

class Server
{
private:
    std::string           _password;
    std::string           _serverName;
    std::map<int, Client> _clients;

public:
    Server(int port, const std::string &password);
    Server(const Server &other);
    Server &operator=(const Server &other);
    ~Server();

    /*
     * Needed by PASS
     */
    const std::string &getPassword() const;

    /*
     * Needed for IRC replies
     */
    const std::string &serverName() const;

    /*
     * Needed by NICK / PRIVMSG
     */
    Client *findNick(const std::string &nickname);
    bool nickInUse(const std::string &nickname) const;

    /*
     * Needed for testing several clients without networking.
     */
    void addClient(const Client &client);
    void removeClient(int fd);

    /*
     * Needed by CommandHandler to produce responses.
     */
    void queue(Client &client, const std::string &message);

    void numeric(
        Client &client,
        int code,
        const std::string &message
    );
};

#endif