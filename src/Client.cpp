#include "../incl/Client.hpp"


Client::Client()
    : fd(-1),
      passOk(false),
      registered(false),
      quit(false)
{
}

Client::Client(int clientFd, const std::string &clientHost)
    : fd(clientFd),
      host(clientHost),
      passOk(false),
      registered(false),
      quit(false)
{
}

Client::Client(const Client &other)
    : fd(other.fd),
      host(other.host),
      nick(other.nick),
      user(other.user),
      realname(other.realname),
      inbuf(other.inbuf),
      outbuf(other.outbuf),
      passOk(other.passOk),
      registered(other.registered),
      quit(other.quit),
      channels(other.channels)
{
}

Client &Client::operator=(const Client &other)
{
    if (this != &other)
    {
        fd = other.fd;
        host = other.host;
        nick = other.nick;
        user = other.user;
        realname = other.realname;
        inbuf = other.inbuf;
        outbuf = other.outbuf;
        passOk = other.passOk;
        registered = other.registered;
        quit = other.quit;
        channels = other.channels;
    }

    return *this;
}

Client::~Client()
{
}

bool Client::isPasswordAccepted() const
{
    return passOk;
}

void Client::setPasswordAccepted(bool value)
{
    passOk = value;
}

bool Client::isRegistered() const
{
    return registered;
}

void Client::setRegistered(bool value)
{
    registered = value;
}

const std::string &Client::getNickname() const
{
    return nick;
}

void Client::setNickname(const std::string &nickname)
{
    nick = nickname;
}

const std::string &Client::getUsername() const
{
    return user;
}

void Client::setUsername(const std::string &username)
{
    user = username;
}

const std::string &Client::getRealname() const
{
    return realname;
}

void Client::setRealname(const std::string &name)
{
    realname = name;
}

const std::string &Client::getHostname() const
{
    return host;
}

std::string Client::prefix() const
{
    return nick + "!" + user + "@" + host;
}