#include "../incl/Server.hpp"
#include "../incl/Utils.hpp"

#include <sstream>

Server::Server(
    int port,
    const std::string &password
)
    : _password(password),
      _serverName("ircserv.local")
{
    (void)port;
}

Server::Server(const Server &other)
    : _password(other._password),
      _serverName(other._serverName),
      _clients(other._clients)
{
}

Server &Server::operator=(const Server &other)
{
    if (this != &other)
    {
        _password = other._password;
        _serverName = other._serverName;
        _clients = other._clients;
    }

    return *this;
}

Server::~Server()
{
}

/*
 * PASS handler needs this.
 */
const std::string &Server::getPassword() const
{
    return _password;
}

/*
 * Numeric replies need server name.
 */
const std::string &Server::serverName() const
{
    return _serverName;
}

/*
 * Find a connected client by nickname.
 *
 * IRC nick comparison should be case-insensitive.
 */
Client *Server::findNick(const std::string &nickname)
{
    std::map<int, Client>::iterator it;

    for (it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (toUpper(it->second.getNickname())
            == toUpper(nickname))
        {
            return &it->second;
        }
    }

    return NULL;
}

bool Server::nickInUse(const std::string &nickname) const
{
    std::map<int, Client>::const_iterator it;

    for (it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (toUpper(it->second.getNickname())
            == toUpper(nickname))
        {
            return true;
        }
    }

    return false;
}

/*
 * Only for now / unit tests.
 *
 * Later Person 1 will add Client automatically
 * after accept().
 */
void Server::addClient(const Client &client)
{
    _clients[client.fd] = client;
}

void Server::removeClient(int fd)
{
    _clients.erase(fd);
}

/*
 * No send().
 *
 * CommandHandler only creates output.
 * Networking layer will send outbuf later.
 */
void Server::queue(
    Client &client,
    const std::string &message
)
{
    client.outbuf += message;
}

/*
 * Generates IRC numeric response.
 */
void Server::numeric(
    Client &client,
    int code,
    const std::string &message
)
{
    std::ostringstream response;

    response << ":" << _serverName << " ";

    if (code < 10)
        response << "00";
    else if (code < 100)
        response << "0";

    response << code << " ";

    if (client.getNickname().empty())
        response << "*";
    else
        response << client.getNickname();

    response << " ";
    response << message;
    response << "\r\n";

    queue(client, response.str());
}