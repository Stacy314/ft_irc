#include "../incl/Server.hpp"
#include "../incl/Parser.hpp"
#include <cerrno>

Server::Server(size_t port, const std::string &password) : _handler(*this) 
{
	this->port = port;
	this->password = password;
	this->serverFd = -1;
	this->runing = false;
	this->_serverName = "ircserv";

}

Server::~Server(){}

int Server::getPort()
{
	return (this->port);
}

void Server::setPort(size_t  port)
{
	this->port = port;
}

void Server::setPassword(std::string password)
{
	this->password = password;
}

const std::string &Server::getPassword() const 
{
    return password;
}

const std::string &Server::serverName() const 
{
    return _serverName;
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
	sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);

	newfd = accept(serverFd, reinterpret_cast<sockaddr *>(&clientAddr),
		&clientAddrLen);
	if (newfd == -1)
		return;

	if (fcntl(newfd, F_SETFL, O_NONBLOCK) == -1)
	{
		close(newfd);
		throw std::runtime_error("Failed to create nonblocksocket");
	}
	const unsigned char *address =
		reinterpret_cast<const unsigned char *>(&clientAddr.sin_addr.s_addr);
	std::ostringstream host;
	host << static_cast<unsigned int>(address[0]) << "."
		<< static_cast<unsigned int>(address[1]) << "."
		<< static_cast<unsigned int>(address[2]) << "."
		<< static_cast<unsigned int>(address[3]);
	Client client(newfd, host.str());
	clients.insert(std::make_pair(newfd, client));
	addPollfd(newfd);
}

void Server::removeChannel(const std::string &name)
{
	_channels.erase(name);
}

void Server::disconnectClient(Client &client)
{
    int fd = client.getFd();

    std::map<std::string, Channel>::iterator channelIt =
        _channels.begin();

    while (channelIt != _channels.end())
    {
        ChannelResult result =
            channelIt->second.removeMember(&client);

        if (result == CHANNEL_EMPTY)
        {
            std::map<std::string, Channel>::iterator empty =
                channelIt++;

            _channels.erase(empty);
        }
        else
        {
            ++channelIt;
        }
    }

    close(fd);

    for (std::vector<pollfd>::iterator it = pollfds.begin();
         it != pollfds.end();
         ++it)
    {
        if (it->fd == fd)
        {
            pollfds.erase(it);
            break;
        }
    }

    clients.erase(fd);
}

void Server::updatePollEvents(int fd)
{
	for (std::vector<pollfd>::iterator pfd = pollfds.begin(); pfd != pollfds.end(); ++pfd)
	{
		if (pfd->fd == fd)
		{
			pfd->events = POLLIN;
			std::map<int, Client>::iterator clientIt = clients.find(fd);
			if (clientIt != clients.end() && !clientIt->second.outbuf.empty())
				pfd->events |= POLLOUT;
			return;
		}
	}
}

// void Server::flushClient(int fd)
// {
// 	std::map<int, Client>::iterator it = clients.find(fd);
// 	if (it == clients.end() || it->second.outbuf.empty())
// 		return;

// 	ssize_t bytes = send(fd, it->second.outbuf.data(), it->second.outbuf.size(), 0);
// 	if (bytes > 0)
// 	{
// 		it->second.outbuf.erase(0, bytes);
// 		updatePollEvents(fd);
// 		return;
// 	}
// 	if (bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
// 		return;
// 	disconnectClient(Client &client);
// }

void Server::flushClient(int fd) {
	std::map<int, Client>::iterator it = clients.find(fd);

    if (it == clients.end() || it->second.outbuf.empty())
        return;

    ssize_t bytes = send(fd, it->second.outbuf.data(), it->second.outbuf.size(), 0);

    if (bytes > 0) {
        it->second.outbuf.erase(0, bytes);
        updatePollEvents(fd);
    }
}

// void Server::reciveCom(int fd)
// {
// 	char buffer[1024];
// 	std::map<int, Client>::iterator it = clients.find(fd);
// 	if (it == clients.end())
// 		return;

// 	ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
// 	if (bytes > 0)
// 	{
// 		Client &client = it->second;
// 		client.inbuf.append(buffer, bytes);

// 		std::string::size_type end;
// 		while ((end = client.inbuf.find("\r\n")) != std::string::npos)
// 		{
// 			std::string line = client.inbuf.substr(0, end);
// 			client.inbuf.erase(0, end + 2);
// 			if (!line.empty())
// 			{
// 				_handler.execute(client, Parser::parse(line));
// 			}
// 		}
// 		return;
// 	}

// 	if (bytes == -1 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
// 		return;

// 	disconnectClient(Client &client);
// }

void Server::reciveCom(int fd)
{
    char buffer[1024];
    std::map<int, Client>::iterator it = clients.find(fd);

    if (it == clients.end())
        return;

    ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);

    if (bytes > 0)
    {
        Client &client = it->second;

        client.inbuf.append(
            buffer,
            static_cast<std::size_t>(bytes)
        );

        std::string::size_type end;

        while ((end = client.inbuf.find("\r\n"))
            != std::string::npos)
        {
            std::string line =
                client.inbuf.substr(0, end);

            client.inbuf.erase(0, end + 2);

            if (!line.empty())
                _handler.execute(
                    client,
                    Parser::parse(line)
                );
        }

        return;
    }

    if (bytes == 0)
    {
        disconnectClient(it->second);
        return;
    }
}

void Server::pollLoop()
{
    while (runing)
    {
        if (poll(&pollfds[0], pollfds.size(), -1) == -1)
        {
            if (errno == EINTR)
                continue;
            throw std::runtime_error("Poll failed!");
        }

        for (size_t i = 0; i < pollfds.size(); )
        {
            int fd = pollfds[i].fd;
            short revents = pollfds[i].revents;

            if (fd == serverFd)
            {
                if (revents & POLLIN)
                    acceptClient();

                ++i;
                continue;
            }

            if (revents & POLLIN)
                reciveCom(fd);

            if (clients.find(fd) != clients.end()
                && (revents & POLLOUT))
            {
                flushClient(fd);
            }

            std::map<int, Client>::iterator clientIt =
                clients.find(fd);

            if (clientIt != clients.end()
                && (revents & (POLLERR | POLLHUP | POLLNVAL)))
            {
                disconnectClient(clientIt->second);
            }

            if (i < pollfds.size()
                && pollfds[i].fd == fd)
            {
                ++i;
            }
        }
    }
}

void Server::start()
{
	if (password.empty())
		throw std::invalid_argument("Password cant be empty!");
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		throw std::runtime_error("Failed to ignore SIGPIPE");
	createSocket();
	bindSocket();
	listenSocket();
	addPollfd(serverFd);
	runing = true;
	pollLoop();
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

    for (it = clients.begin(); it != clients.end(); ++it) {
        if (toUpper(it->second.getNickname())
            == toUpper(nickname)) {
            return &it->second;
        }
    }
    return NULL;
}

void Server::queue(Client &client, const std::string &message)
{
    client.outbuf += message;
	updatePollEvents(client.getFd());
}

void Server::broadcastQuit(Client &client, const std::string &message) {
    std::set<Client *> receivers;

    std::map<std::string, Channel>::iterator it = _channels.begin();

    while (it != _channels.end()) {
        if (it->second.isMember(&client)) {
            const std::vector<Client *> &members = it->second.getMembers();

            for (std::vector<Client *>::const_iterator m = members.begin();
                m != members.end(); ++m) {
                if (*m != &client)
                    receivers.insert(*m);
            }
        }
        ++it;
    }

	for (std::set<Client *>::iterator it = receivers.begin();
		it != receivers.end(); ++it)
	{
		queue(**it, message + "\r\n");
	}
}