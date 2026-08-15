#pragma once 

#include <string>
#include <set>

class Client
{
public:
    int fd;

    std::string host;
    std::string nick;
    std::string user;
    std::string realname;

    std::string inbuf;
    std::string outbuf;

    bool passOk;
    bool registered;
    bool quit;

    std::set<std::string> channels;

    Client();
    Client(int fd, const std::string &host);
    Client(const Client &other);
    Client &operator=(const Client &other);
    ~Client();


    bool isPasswordAccepted() const;
    void setPasswordAccepted(bool value);

    bool isRegistered() const;
    void setRegistered(bool value);

    const std::string &getNickname() const;
    void setNickname(const std::string &nickname);

    const std::string &getUsername() const;
    void setUsername(const std::string &username);

    const std::string &getRealname() const;
    void setRealname(const std::string &name);

    const std::string &getHostname() const;

    std::string prefix() const;
};