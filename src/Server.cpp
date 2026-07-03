#include "../incl/Server.hpp"
#include "../incl/Utils.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>

static Server *g_server = NULL;
static bool g_stop = false;
static void onSignal(int) { g_stop = true; }

Server::Server(int port, const std::string &password)
    : _port(port), _password(password), _listenFd(-1), _running(true) {
    g_server = this;
    (void)g_server;
    signal(SIGINT, onSignal);
    signal(SIGTERM, onSignal);
    setupSocket();
}

Server::~Server() {
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        close(it->first);
    if (_listenFd >= 0)
        close(_listenFd);
}

std::string Server::serverName() const { return "ircserv.local"; }

void Server::setupSocket() {
    _listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenFd < 0)
        throw std::runtime_error("socket failed");
    int yes = 1;
    if (setsockopt(_listenFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
        throw std::runtime_error("setsockopt failed");
    if (fcntl(_listenFd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl failed");
    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(_port);
    if (bind(_listenFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error("bind failed");
    if (listen(_listenFd, SOMAXCONN) < 0)
        throw std::runtime_error("listen failed");
}

void Server::rebuildPollfds() {
    _pfds.clear();
    pollfd p;
    p.fd = _listenFd;
    p.events = POLLIN;
    p.revents = 0;
    _pfds.push_back(p);
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        pollfd c;
        c.fd = it->first;
        c.events = POLLIN;
        if (!it->second.outbuf.empty())
            c.events |= POLLOUT;
        c.revents = 0;
        _pfds.push_back(c);
    }
}

void Server::run() {
    std::cout << "ircserv listening on port " << _port << std::endl;
    while (_running && !g_stop) {
        rebuildPollfds();
        int ready = poll(&_pfds[0], _pfds.size(), 1000);
        if (ready < 0) {
            if (errno == EINTR)
                continue;
            throw std::runtime_error("poll failed");
        }
        if (ready == 0)
            continue;
        std::vector<int> toRead;
        std::vector<int> toWrite;
        for (size_t i = 0; i < _pfds.size(); ++i) {
            if (_pfds[i].revents == 0)
                continue;
            if (_pfds[i].fd == _listenFd && (_pfds[i].revents & POLLIN))
                acceptClient();
            else {
                if (_pfds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
                    disconnectClient(_pfds[i].fd, "connection error");
                else {
                    if (_pfds[i].revents & POLLIN)
                        toRead.push_back(_pfds[i].fd);
                    if (_pfds[i].revents & POLLOUT)
                        toWrite.push_back(_pfds[i].fd);
                }
            }
        }
        for (size_t i = 0; i < toRead.size(); ++i)
            if (_clients.find(toRead[i]) != _clients.end()) readClient(toRead[i]);
        for (size_t i = 0; i < toWrite.size(); ++i)
            if (_clients.find(toWrite[i]) != _clients.end()) writeClient(toWrite[i]);
        std::vector<int> gone;
        for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
            if (it->second.quit) gone.push_back(it->first);
        for (size_t i = 0; i < gone.size(); ++i)
            disconnectClient(gone[i], "quit");
    }
}

void Server::acceptClient() {
    while (true) {
        sockaddr_in addr;
        socklen_t len = sizeof(addr);
        int fd = accept(_listenFd, reinterpret_cast<sockaddr *>(&addr), &len);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            return;
        }
        fcntl(fd, F_SETFL, O_NONBLOCK);
        std::string host = inet_ntoa(addr.sin_addr);
        _clients[fd] = Client(fd, host);
        queue(fd, ":" + serverName() + " NOTICE * :Welcome. Use PASS, NICK and USER to register.\r\n");
    }
}

void Server::readClient(int fd) {
    char buf[512];
    while (true) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n > 0) {
            Client &c = _clients[fd];
            c.inbuf.append(buf, n);
            size_t pos;
            while ((pos = c.inbuf.find('\n')) != std::string::npos) {
                std::string line = trimCrlf(c.inbuf.substr(0, pos + 1));
                c.inbuf.erase(0, pos + 1);
                if (!line.empty())
                    processLine(c, line);
                if (_clients.find(fd) == _clients.end() || c.quit)
                    return;
            }
            if (c.inbuf.size() > 4096) {
                numeric(c, 417, ":Input line too long");
                c.inbuf.clear();
            }
        } else if (n == 0) {
            disconnectClient(fd, "client closed");
            return;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            disconnectClient(fd, "recv error");
            return;
        }
    }
}

void Server::writeClient(int fd) {
    Client &c = _clients[fd];
    while (!c.outbuf.empty()) {
        ssize_t n = send(fd, c.outbuf.c_str(), c.outbuf.size(), 0);
        if (n > 0)
            c.outbuf.erase(0, static_cast<size_t>(n));
        else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return;
        else {
            disconnectClient(fd, "send error");
            return;
        }
    }
}

void Server::disconnectClient(int fd, const std::string &reason) {
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;
    Client c = it->second;
    std::string msg = ":" + c.prefix() + " QUIT :" + reason + "\r\n";
    for (std::set<std::string>::iterator ci = c.channels.begin(); ci != c.channels.end(); ++ci) {
        std::map<std::string, Channel>::iterator chIt = _channels.find(*ci);
        if (chIt != _channels.end()) {
            broadcast(chIt->second, msg, fd);
            chIt->second.members.erase(fd);
            chIt->second.operators.erase(fd);
            chIt->second.invited.erase(fd);
        }
    }
    close(fd);
    _clients.erase(it);
    std::vector<std::string> empty;
    for (std::map<std::string, Channel>::iterator ch = _channels.begin(); ch != _channels.end(); ++ch)
        if (ch->second.members.empty()) empty.push_back(ch->first);
    for (size_t i = 0; i < empty.size(); ++i)
        _channels.erase(empty[i]);
}

void Server::queue(int fd, const std::string &msg) {
    if (_clients.find(fd) != _clients.end())
        _clients[fd].outbuf += msg;
}

void Server::numeric(Client &c, int code, const std::string &msg) {
    std::ostringstream oss;
    oss << ":" << serverName() << " ";
    if (code < 10) oss << "00";
    else if (code < 100) oss << "0";
    oss << code << " " << (c.nick.empty() ? "*" : c.nick) << " " << msg << "\r\n";
    queue(c.fd, oss.str());
}

void Server::broadcast(Channel &ch, const std::string &msg, int exceptFd) {
    for (std::set<int>::iterator it = ch.members.begin(); it != ch.members.end(); ++it)
        if (*it != exceptFd) queue(*it, msg);
}

Client *Server::findNick(const std::string &nick) {
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        if (toUpper(it->second.nick) == toUpper(nick)) return &it->second;
    return NULL;
}

bool Server::nickInUse(const std::string &nick) const {
    for (std::map<int, Client>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
        if (toUpper(it->second.nick) == toUpper(nick)) return true;
    return false;
}

void Server::processLine(Client &c, const std::string &line) {
    std::vector<std::string> p = split(line);
    if (p.empty()) return;
    std::string cmd = toUpper(p[0]);
    if (cmd == "PASS") cmdPass(c, p);
    else if (cmd == "NICK") cmdNick(c, p);
    else if (cmd == "USER") cmdUser(c, p);
    else if (cmd == "JOIN") cmdJoin(c, p);
    else if (cmd == "PRIVMSG" || cmd == "NOTICE") cmdPrivmsg(c, p);
    else if (cmd == "PART") cmdPart(c, p);
    else if (cmd == "QUIT") cmdQuit(c, p);
    else if (cmd == "PING") cmdPing(c, p);
    else if (cmd == "TOPIC") cmdTopic(c, p);
    else if (cmd == "KICK") cmdKick(c, p);
    else if (cmd == "INVITE") cmdInvite(c, p);
    else if (cmd == "MODE") cmdMode(c, p);
    else if (cmd == "CAP") cmdCap(c, p);
    else if (cmd == "WHO") cmdWho(c, p);
    else if (cmd == "PONG") {}
    else numeric(c, 421, p[0] + " :Unknown command");
}

void Server::tryRegister(Client &c) {
    if (!c.registered && c.passOk && !c.nick.empty() && !c.user.empty()) {
        c.registered = true;
        numeric(c, 1, ":Welcome to ircserv " + c.prefix());
        numeric(c, 2, ":Your host is " + serverName());
        numeric(c, 3, ":This server was created for 42 ft_irc");
        numeric(c, 4, serverName() + " 0.1 o o");
        numeric(c, 375, ":- ircserv Message of the day -");
        numeric(c, 372, ":- Have fun and write clean C++98.");
        numeric(c, 376, ":End of MOTD command");
    }
}

void Server::cmdPass(Client &c, const std::vector<std::string> &p) {
    if (c.registered) { numeric(c, 462, ":You may not reregister"); return; }
    if (p.size() < 2) { numeric(c, 461, "PASS :Not enough parameters"); return; }
    if (p[1] != _password) { numeric(c, 464, ":Password incorrect"); return; }
    c.passOk = true;
    tryRegister(c);
}

void Server::cmdNick(Client &c, const std::vector<std::string> &p) {
    if (p.size() < 2) { numeric(c, 431, ":No nickname given"); return; }
    if (!validNick(p[1])) { numeric(c, 432, p[1] + " :Erroneous nickname"); return; }
    if (nickInUse(p[1]) && toUpper(c.nick) != toUpper(p[1])) { numeric(c, 433, p[1] + " :Nickname is already in use"); return; }
    std::string old = c.nick;
    c.nick = p[1];
    if (c.registered && old != c.nick) {
        std::string msg = ":" + old + "!" + c.user + "@" + c.host + " NICK :" + c.nick + "\r\n";
        queue(c.fd, msg);
        for (std::set<std::string>::iterator it = c.channels.begin(); it != c.channels.end(); ++it)
            broadcast(_channels[*it], msg, c.fd);
    }
    tryRegister(c);
}

void Server::cmdUser(Client &c, const std::vector<std::string> &p) {
    if (c.registered) { numeric(c, 462, ":You may not reregister"); return; }
    if (p.size() < 5) { numeric(c, 461, "USER :Not enough parameters"); return; }
    c.user = p[1];
    c.realname = p[4];
    tryRegister(c);
}

void Server::cmdJoin(Client &c, const std::vector<std::string> &p) {
    if (!c.registered) { numeric(c, 451, ":You have not registered"); return; }
    if (p.size() < 2) { numeric(c, 461, "JOIN :Not enough parameters"); return; }
    std::string name = p[1];
    if (name.empty() || name[0] != '#') { numeric(c, 403, name + " :No such channel"); return; }
    std::string key = p.size() > 2 ? p[2] : "";
    if (_channels.find(name) == _channels.end())
        _channels[name] = Channel(name);
    Channel &ch = _channels[name];
    if (ch.has(c.fd)) return;
    if (ch.inviteOnly && ch.invited.find(c.fd) == ch.invited.end()) { numeric(c, 473, name + " :Cannot join channel (+i)"); return; }
    if (!ch.key.empty() && ch.key != key) { numeric(c, 475, name + " :Cannot join channel (+k)"); return; }
    if (ch.userLimit > 0 && static_cast<int>(ch.members.size()) >= ch.userLimit) { numeric(c, 471, name + " :Cannot join channel (+l)"); return; }
    bool first = ch.members.empty();
    ch.members.insert(c.fd);
    c.channels.insert(name);
    ch.invited.erase(c.fd);
    if (first) ch.operators.insert(c.fd);
    std::string msg = ":" + c.prefix() + " JOIN :" + name + "\r\n";
    broadcast(ch, msg, -1);
    if (!ch.topic.empty()) numeric(c, 332, name + " :" + ch.topic);
    else numeric(c, 331, name + " :No topic is set");
    sendNames(c, ch);
}

void Server::sendNames(Client &c, Channel &ch) {
    std::string names;
    for (std::set<int>::iterator it = ch.members.begin(); it != ch.members.end(); ++it) {
        if (_clients.find(*it) == _clients.end()) continue;
        if (!names.empty()) names += " ";
        if (ch.isOp(*it)) names += "@";
        names += _clients[*it].nick;
    }
    numeric(c, 353, "= " + ch.name + " :" + names);
    numeric(c, 366, ch.name + " :End of /NAMES list");
}

void Server::cmdPrivmsg(Client &c, const std::vector<std::string> &p) {
    if (!c.registered) { numeric(c, 451, ":You have not registered"); return; }
    if (p.size() < 2) { numeric(c, 411, ":No recipient given"); return; }
    if (p.size() < 3) { numeric(c, 412, ":No text to send"); return; }
    std::string target = p[1];
    std::string msg = ":" + c.prefix() + " PRIVMSG " + target + " :" + p[2] + "\r\n";
    if (!target.empty() && target[0] == '#') {
        if (_channels.find(target) == _channels.end()) { numeric(c, 403, target + " :No such channel"); return; }
        Channel &ch = _channels[target];
        if (!ch.has(c.fd)) { numeric(c, 442, target + " :You're not on that channel"); return; }
        broadcast(ch, msg, c.fd);
    } else {
        Client *dst = findNick(target);
        if (!dst) { numeric(c, 401, target + " :No such nick"); return; }
        queue(dst->fd, msg);
    }
}

void Server::cmdPart(Client &c, const std::vector<std::string> &p) {
    if (p.size() < 2) { numeric(c, 461, "PART :Not enough parameters"); return; }
    std::string name = p[1];
    if (_channels.find(name) == _channels.end()) { numeric(c, 403, name + " :No such channel"); return; }
    Channel &ch = _channels[name];
    if (!ch.has(c.fd)) { numeric(c, 442, name + " :You're not on that channel"); return; }
    std::string reason = p.size() > 2 ? p[2] : "leaving";
    broadcast(ch, ":" + c.prefix() + " PART " + name + " :" + reason + "\r\n", -1);
    ch.members.erase(c.fd); ch.operators.erase(c.fd); c.channels.erase(name);
    if (ch.members.empty()) _channels.erase(name);
}

void Server::cmdQuit(Client &c, const std::vector<std::string> &p) {
    (void)p;
    c.quit = true;
}

void Server::cmdPing(Client &c, const std::vector<std::string> &p) {
    std::string token = p.size() > 1 ? p[1] : serverName();
    queue(c.fd, ":" + serverName() + " PONG " + serverName() + " :" + token + "\r\n");
}

void Server::cmdTopic(Client &c, const std::vector<std::string> &p) {
    if (p.size() < 2) { numeric(c, 461, "TOPIC :Not enough parameters"); return; }
    std::string name = p[1];
    if (_channels.find(name) == _channels.end()) { numeric(c, 403, name + " :No such channel"); return; }
    Channel &ch = _channels[name];
    if (!ch.has(c.fd)) { numeric(c, 442, name + " :You're not on that channel"); return; }
    if (p.size() == 2) {
        if (ch.topic.empty()) numeric(c, 331, name + " :No topic is set");
        else numeric(c, 332, name + " :" + ch.topic);
        return;
    }
    if (ch.topicOpsOnly && !ch.isOp(c.fd)) { numeric(c, 482, name + " :You're not channel operator"); return; }
    ch.topic = p[2];
    broadcast(ch, ":" + c.prefix() + " TOPIC " + name + " :" + ch.topic + "\r\n", -1);
}

void Server::cmdKick(Client &c, const std::vector<std::string> &p) {
    if (p.size() < 3) { numeric(c, 461, "KICK :Not enough parameters"); return; }
    std::string name = p[1];
    if (_channels.find(name) == _channels.end()) { numeric(c, 403, name + " :No such channel"); return; }
    Channel &ch = _channels[name];
    if (!ch.isOp(c.fd)) { numeric(c, 482, name + " :You're not channel operator"); return; }
    Client *victim = findNick(p[2]);
    if (!victim || !ch.has(victim->fd)) { numeric(c, 441, p[2] + " " + name + " :They aren't on that channel"); return; }
    std::string reason = p.size() > 3 ? p[3] : c.nick;
    broadcast(ch, ":" + c.prefix() + " KICK " + name + " " + victim->nick + " :" + reason + "\r\n", -1);
    ch.members.erase(victim->fd); ch.operators.erase(victim->fd); victim->channels.erase(name);
}

void Server::cmdInvite(Client &c, const std::vector<std::string> &p) {
    if (p.size() < 3) { numeric(c, 461, "INVITE :Not enough parameters"); return; }
    Client *dst = findNick(p[1]);
    std::string name = p[2];
    if (!dst) { numeric(c, 401, p[1] + " :No such nick"); return; }
    if (_channels.find(name) == _channels.end()) { numeric(c, 403, name + " :No such channel"); return; }
    Channel &ch = _channels[name];
    if (!ch.has(c.fd)) { numeric(c, 442, name + " :You're not on that channel"); return; }
    if (!ch.isOp(c.fd)) { numeric(c, 482, name + " :You're not channel operator"); return; }
    ch.invited.insert(dst->fd);
    numeric(c, 341, dst->nick + " " + name);
    queue(dst->fd, ":" + c.prefix() + " INVITE " + dst->nick + " :" + name + "\r\n");
}

void Server::cmdMode(Client &c, const std::vector<std::string> &p) {
    if (p.size() < 2) { numeric(c, 461, "MODE :Not enough parameters"); return; }
    std::string name = p[1];
    if (name.empty() || name[0] != '#') { numeric(c, 502, ":Cannot change mode for other users"); return; }
    if (_channels.find(name) == _channels.end()) { numeric(c, 403, name + " :No such channel"); return; }
    Channel &ch = _channels[name];
    if (p.size() == 2) { numeric(c, 324, name + " " + ch.modes()); return; }
    if (!ch.isOp(c.fd)) { numeric(c, 482, name + " :You're not channel operator"); return; }
    bool add = true;
    size_t arg = 3;
    std::string applied;
    for (size_t i = 0; i < p[2].size(); ++i) {
        char m = p[2][i];
        if (m == '+') { add = true; if (applied.empty() || applied[applied.size()-1] == '-') applied += '+'; continue; }
        if (m == '-') { add = false; if (applied.empty() || applied[applied.size()-1] == '+') applied += '-'; continue; }
        if (m == 'i') { ch.inviteOnly = add; applied += m; }
        else if (m == 't') { ch.topicOpsOnly = add; applied += m; }
        else if (m == 'k') {
            if (add) { if (arg >= p.size()) { numeric(c, 461, "MODE :Not enough parameters"); return; } ch.key = p[arg++]; }
            else ch.key.clear();
            applied += m;
        } else if (m == 'l') {
            if (add) { if (arg >= p.size() || !isNumber(p[arg])) { numeric(c, 461, "MODE :Not enough parameters"); return; } ch.userLimit = std::atoi(p[arg++].c_str()); }
            else ch.userLimit = 0;
            applied += m;
        } else if (m == 'o') {
            if (arg >= p.size()) { numeric(c, 461, "MODE :Not enough parameters"); return; }
            Client *u = findNick(p[arg++]);
            if (!u || !ch.has(u->fd)) { numeric(c, 441, "* " + name + " :They aren't on that channel"); return; }
            if (add) ch.operators.insert(u->fd); else ch.operators.erase(u->fd);
            applied += m;
        }
    }
    if (!applied.empty())
        broadcast(ch, ":" + c.prefix() + " MODE " + name + " " + p[2] + "\r\n", -1);
}

void Server::cmdCap(Client &c, const std::vector<std::string> &p) {
    if (p.size() < 2) return;
    std::string sub = toUpper(p[1]);
    if (sub == "LS") queue(c.fd, ":" + serverName() + " CAP * LS :\r\n");
    else if (sub == "REQ") queue(c.fd, ":" + serverName() + " CAP * ACK :" + (p.size() > 2 ? p[2] : "") + "\r\n");
}

void Server::cmdWho(Client &c, const std::vector<std::string> &p) {
    std::string name = p.size() > 1 ? p[1] : "";
    if (_channels.find(name) != _channels.end()) {
        Channel &ch = _channels[name];
        for (std::set<int>::iterator it = ch.members.begin(); it != ch.members.end(); ++it) {
            Client &u = _clients[*it];
            numeric(c, 352, name + " " + u.user + " " + u.host + " " + serverName() + " " + u.nick + " H :0 " + u.realname);
        }
    }
    numeric(c, 315, name + " :End of /WHO list");
}
