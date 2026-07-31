#include "CommandHandler.hpp"
#include "CommandHandler.hpp"

CommandHandler::CommandHandler() {}

CommandHandler::CommandHandler(const CommandHandler &other) {(void)other;}

CommandHandler &CommandHandler::operator=(const CommandHandler &other) {
    (void)other;
    return (*this);
}

CommandHandler::~CommandHandler() {}

void CommandHandler::handlePass(Client &client, const Command &command){
    (void)client;
    (void)command;
}

void CommandHandler::handleNick(Client &client, const Command &command) {
    (void)client;
    (void)command;
}

void CommandHandler::handleUser(Client &client, const Command &command) {
    (void)client;
    (void)command;
}

void CommandHandler::handleJoin(Client &client, const Command &command) {
    (void)client;
    (void)command;
}

void CommandHandler::handlePrivmsg(Client &client, const Command &command) {
    (void)client;
    (void)command;
}