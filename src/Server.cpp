#include "../incl/Server.hpp"


Server::Server()
{
	this->port = 0;
	this->password = "0";
	this->serverFd = -1;
	this->runing = false;

}

Server::Server(size_t port, const std::string &password)
{
	this->port = port;
	this->password = password;
	this->serverFd = -1;
	this->runing = false;

}

Server::Server(const Server& obj)
{	
	*this = obj;
}

Server& Server::operator=(const Server&  obj)
{
	if (this == &obj)
		return *this;
	this->port = obj.port;
	this->password = obj.password;
	return *this;
}

Server::~Server(){}

int Server::getPort()
{
	return (this->port);
}

std::string Server::getPassword()
{
	return (this->password);
}

void Server::setPort(size_t  port)
{
	this->port = port;
}

void Server::setPassword(std::string password)
{
	this->password = password;
}

void Server::createSocket()
{
	serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd == -1)
		throw std::runtime_error("Failed to create socket");
	if (fcntl(serverFd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("Failed to create nonblocksocket");
	int opt = 1;
	if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
			throw std::runtime_error("Failed to set socket option!");
}

void  Server::bindSocket()
{
	sockaddr_in serverAdr;
	serverAdr.sin_family = AF_INET;
	serverAdr.sin_port = htons(port);
	serverAdr.sin_addr.s_addr = INADDR_ANY;

	if (bind(serverFd, (sockaddr*)&serverAdr, sizeof(serverAdr)) == -1)
		throw std::runtime_error("Failed to bind socket!");
}

void  Server::listenSocket()
{
	if (listen(serverFd, SOMAXCONN) == -1)
		throw std::runtime_error("Failed to listen socket!");
}

void Server::addPollfd(int fd)
{
	pollfd pfd;

	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;

	pollfds.push_back(pfd);
}

void	Server::acceptClient()
{
	int newfd;

	newfd = accept(serverFd, NULL, NULL);
	if (newfd == -1)
		throw std::runtime_error("Failed to accept client!");
	if (fcntl(newfd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("Failed to create nonblocksocket");
	Client client(newfd);
	clients.insert(std::make_pair(newfd, client));
	addPollfd(newfd);
}

void Server::reciveCom(int fd)
{
	char buffer[1024];
	
	recv(fd, buffer, sizeof(buffer))
}

void  Server::pollLoop()
{
	while (runing)
	{
		if(poll(pollfds.data(), pollfds.size(), -1) == -1)
		{
			// if (errno)
			throw std::runtime_error("Poll failed!");
		}
		size_t count = pollfds.size();
		for (size_t i = 0; i < count; i++)
		{
			if (pollfds[i].revents & POLLIN)
			{
				if (pollfds[i].fd == serverFd)
					acceptClient();
				else
					reciveCom(pollfds[i].fd);
			}
		}

	}
}

void Server::start()
{
	if (password.empty())
		throw std::invalid_argument("Password cant be empty!");
	createSocket();
	bindSocket();
	listenSocket();
	addPollfd(serverFd);
	runing = true;
	pollLoop();
	
#include "../incl/Utils.hpp"

Server::Server(int port, const std::string &password) : _password(password), _serverName("ircserv.local") {
    (void)port;
}

Server::~Server() {}

const std::string &Server::getPassword() const {
    return _password;
}


const std::string &Server::serverName() const {
    return _serverName;
}

Channel *Server::findChannel(const std::string &name)
{
    std::map<std::string, Channel>::iterator it =
        _channels.find(name);

    if (it == _channels.end())
        return NULL;

    return &(it->second);
}
Channel &Server::createChannel(const std::string &name) {
    std::pair<
        std::map<std::string, Channel>::iterator,
        bool
    > result = _channels.insert(
        std::make_pair(name, Channel(name))
    );

    return result.first->second;
}

Client *Server::findNick(const std::string &nickname) {
    std::map<int, Client>::iterator it;

    for (it = _clients.begin(); it != _clients.end(); ++it) {
        if (toUpper(it->second.getNickname())
            == toUpper(nickname)) {
            return &it->second;
        }
    }
    return NULL;
}

void Server::queue(
    Client &client,
    const std::string &message)
{
    client.outbuf += message;
}