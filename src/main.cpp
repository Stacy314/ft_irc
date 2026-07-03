#include "../incl/Server.hpp"
#include "../incl/Utils.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>

//You must not develop an IRC client.
//You must not implement server-to-server communication.

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }
    std::string portStr = argv[1];
    if (!isNumber(portStr)) {
        std::cerr << "Error: port must be a number" << std::endl;
        return 1;
    }
    int port = std::atoi(portStr.c_str());
    if (port < 1 || port > 65535) {
        std::cerr << "Error: port must be between 1 and 65535" << std::endl;
        return 1;
    }
    std::string password = argv[2];
    if (password.empty()) {
        std::cerr << "Error: password cannot be empty" << std::endl;
        return 1;
    }
    try {
        Server server(port, password);
        server.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
