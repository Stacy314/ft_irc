#pragma once 

#include <string>
#include <set>

class Client
{
private:
	bool userReceived;
    int fd;
	std::string host;
    std::string nick;
    std::string user;
	std::string fullname;
    bool pass;
    bool registered;
    bool quit;
	
public:

    Client();
    Client(int fd, const std::string &host);
    Client(const Client &other);
    Client &operator=(const Client &other);
    ~Client();

  	std::string outbuf;
  	std::string inbuf;
    bool isPasswordAccepted() const;
    void setPasswordAccepted(bool value);
    bool isRegistered() const;
    void setRegistered(bool value);
    const std::string &getNickname() const;
    void setNickname(const std::string &nickname);
    const std::string &getUsername() const;
    void setUsername(const std::string &username);
    const std::string &getRealname() const;
    void setHostname(const std::string &hostname);
    const std::string &getHostname() const;
    void setRealname(const std::string &name);
	bool isUserReceived() const;
	void setUserReceived(bool value);
    int getFd() const;
};
