#include "../incl/Server.hpp"
#include "../incl/Utils.hpp"
#include "../incl/Parser.hpp"
#include "../incl/Command.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>

//You must not develop an IRC client.
//You must not implement server-to-server communication.
//Forking is prohibited.

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


//#include "../incl/Parser.hpp"
//#include "../incl/Command.hpp"
//#include <iostream>

//static void printResult(const Command &cmd)
//{
//    std::cout << "Command: " << cmd.getName() << std::endl;

//    const std::vector<std::string> &params = cmd.getParameters();

//    for (std::size_t i = 0; i < params.size(); ++i)
//    {
//        std::cout << "param[" << i << "] = "
//                  << params[i] << std::endl;
//    }
//}

//int main()
//{
//    Command cmd = Parser::parse(
//        "PRIVMSG #general :Hello everyone"
//    );

//    printResult(cmd);

//    return 0;
//}