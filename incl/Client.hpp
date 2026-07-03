#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <set>

class Client {
public:
    Client();
    Client(int fd, const std::string &host);

    int fd;
    std::string host;
    std::string nick;
    std::string user;
    std::string realname;
    bool passOk;
    bool registered;
    bool quit;
    std::string inbuf;
    std::string outbuf;
    std::set<std::string> channels;

    std::string prefix() const;
};

#endif
