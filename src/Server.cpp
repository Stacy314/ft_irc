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

void  Server::pollLoop()
{
	while (runing)
	{
		if(poll(pollfds.data(), pollfds.size(), -1) == -1)
			throw std::runtime_error("Poll failed!");
		for (size_t i = 0; i < pollfds.size(); i++)
		{
			if (pollfds[i].revents & POLLIN)
			{
				if (pollfds[i].fd == serverFd)
					acceptClient();
				else
					std::cout << "NASTYa\n";
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
	addPollfd();
	runing = true;
	pollLoop();
	
}