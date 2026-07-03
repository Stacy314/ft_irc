#include "../incl/Client.hpp"

Client::Client() : fd(-1), passOk(false), registered(false), quit(false) {}

Client::Client(int f, const std::string &h)
    : fd(f), host(h), passOk(false), registered(false), quit(false) {}

std::string Client::prefix() const {
    std::string n = nick.empty() ? "*" : nick;
    std::string u = user.empty() ? "unknown" : user;
    return n + "!" + u + "@" + host;
}
