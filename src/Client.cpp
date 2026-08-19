#include "../incl/Client.hpp"

Client::Client()
    : userReceived(false),
	  fd(-1),
      pass(false),
      registered(false),
      quit(false)
	  
{}

Client::Client(int clientFd, const std::string &clientHost)
    : userReceived(false),
	  fd(clientFd),
      host(clientHost),
      pass(false),
      registered(false),
      quit(false)
{}

Client::Client(const Client &other)
    : userReceived(other.userReceived),
	  fd(other.fd),
      host(other.host),
      nick(other.nick),
      user(other.user),
	  fullname(other.fullname),
      pass(other.pass),
      registered(other.registered),
      quit(other.quit),
	  outbuf(other.outbuf),
	  inbuf(other.inbuf)
{}

Client &Client::operator=(const Client &other) {
    if (this != &other) {
		userReceived = other.userReceived,
        fd = other.fd;
        host = other.host;
        nick = other.nick;
        user = other.user;
		fullname = other.fullname;
        pass = other.pass;
        registered = other.registered;
        quit = other.quit;
        outbuf = other.outbuf;
		inbuf = other.inbuf;
    }
    return *this;
}

Client::~Client() {}

bool Client::isPasswordAccepted() const {
    return pass;
}

void Client::setPasswordAccepted(bool value) {
    pass = value;
}

bool Client::isRegistered() const {
    return registered;
}

void Client::setRegistered(bool value) {
    registered = value;
}

const std::string &Client::getNickname() const {
    return nick;
}

void Client::setNickname(const std::string &nickname) {
    nick = nickname;
}

const std::string &Client::getUsername() const {
    return user;
}

void Client::setUsername(const std::string &username) {
    user = username;
}

const std::string &Client::getRealname() const {
    return fullname;
}

void Client::setRealname(const std::string &name) {
    fullname = name;
}

//const std::string &Client::getHostname() const
//{
//    return host;
//}

void Client::setUserReceived(bool value) {
    userReceived = value;
}

bool Client::isUserReceived() const {
    return userReceived;
}
