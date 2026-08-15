#include "../incl/CommandTests.hpp"
#include "../incl/CommandHandler.hpp"
#include "../incl/Command.hpp"
#include "../incl/Parser.hpp"
#include "../incl/Client.hpp"
#include "../incl/Server.hpp"

#include <iostream>
#include <string>

static void printTest(const std::string &name, bool result)
{
    if (result)
        std::cout << "[OK]   " << name << std::endl;
    else
        std::cout << "[FAIL] " << name << std::endl;
}

static void testPass()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    Command command = Parser::parse("PASS secret");

    handler.execute(client, command);

    printTest(
        "PASS correct password",
        client.isPasswordAccepted()
    );
}

static void testWrongPass()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    Command command = Parser::parse("PASS wrong");

    handler.execute(client, command);

    printTest(
        "PASS wrong password",
        !client.isPasswordAccepted()
    );
}

static void testInvalidNick()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse("NICK 123bob")
    );

    printTest(
        "Invalid nickname -> 432",
        client.outbuf.find("432") != std::string::npos
    );
}

static void testDuplicateNick()
{
    Server server(6667, "secret");
    CommandHandler handler(server);

    Client client1;
    Client client2;

    handler.execute(client1, Parser::parse("NICK bob"));

    handler.execute(client2, Parser::parse("NICK bob"));

    printTest(
        "Duplicate nickname -> 433",
        client2.outbuf.find("433") != std::string::npos
    );
}

static void testPassAfterRegistration()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(client, Parser::parse("PASS secret"));
    handler.execute(client, Parser::parse("NICK bob"));
    handler.execute(
        client,
        Parser::parse("USER bob 0 * :Bob Smith")
    );

    client.outbuf.clear();

    handler.execute(
        client,
        Parser::parse("PASS secret")
    );

    printTest(
        "PASS after registration -> 462",
        client.outbuf.find("462") != std::string::npos
    );
}

static void testUserAfterRegistration()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(client, Parser::parse("PASS secret"));
    handler.execute(client, Parser::parse("NICK bob"));
    handler.execute(
        client,
        Parser::parse("USER bob 0 * :Bob Smith")
    );

    client.outbuf.clear();

    handler.execute(
        client,
        Parser::parse("USER alice 0 * :Alice Smith")
    );

    printTest(
        "USER after registration -> 462",
        client.outbuf.find("462") != std::string::npos
    );
}

static void testUnknownCommand()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse("HELLO world")
    );

    printTest(
        "Unknown command -> 421",
        client.outbuf.find("421") != std::string::npos
    );
}

static void testLowercaseCommand()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse("nick bob")
    );

    printTest(
        "Lowercase command works",
        client.getNickname() == "bob"
    );
}

static void testRegistrationWithoutPass()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse("NICK bob")
    );

    handler.execute(
        client,
        Parser::parse("USER bob 0 * :Bob Smith")
    );

    printTest(
        "No registration without PASS",
        !client.isRegistered()
    );
}

static void testRegistrationWithoutNick()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse("PASS secret")
    );

    handler.execute(
        client,
        Parser::parse("USER bob 0 * :Bob Smith")
    );

    printTest(
        "No registration without NICK",
        !client.isRegistered()
    );
}

static void testRegistrationWithoutUser()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse("PASS secret")
    );

    handler.execute(
        client,
        Parser::parse("NICK bob")
    );

    printTest(
        "No registration without USER",
        !client.isRegistered()
    );
}

static void testNick()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    Command command = Parser::parse("NICK bob");

    handler.execute(client, command);

    printTest(
        "NICK sets nickname",
        client.getNickname() == "bob"
    );
}

static void testUser()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    Command command =
        Parser::parse("USER bob 0 * :Bob Smith");

    handler.execute(client, command);

    printTest(
        "USER sets username",
        client.getUsername() == "bob"
    );

    printTest(
        "USER sets realname",
        client.getRealname() == "Bob Smith"
    );
}

static void testRegistration()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse("PASS secret")
    );

    handler.execute(
        client,
        Parser::parse("NICK bob")
    );

    handler.execute(
        client,
        Parser::parse("USER bob 0 * :Bob Smith")
    );

    printTest(
        "Client registered",
        client.isRegistered()
    );

    printTest(
        "001 Welcome generated",
        client.outbuf.find("001") != std::string::npos
    );
}

static void testNickWithoutParameter()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse("NICK")
    );

    printTest(
        "NICK without parameter -> 431",
        client.outbuf.find("431") != std::string::npos
    );
}

static void testPassWithoutParameter()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse("PASS")
    );

    printTest(
        "PASS without parameter -> 461",
        client.outbuf.find("461") != std::string::npos
    );
}

static void testWrongPasswordReply()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse("PASS wrong")
    );

    printTest(
        "Wrong password -> 464",
        client.outbuf.find("464") != std::string::npos
    );
}

static void testUserWithoutParameters()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse("USER bob")
    );

    printTest(
        "USER without enough parameters -> 461",
        client.outbuf.find("461") != std::string::npos
    );
}

void runCommandTests()
{
    std::cout << std::endl;
    std::cout << "==============================" << std::endl;
    std::cout << "       COMMAND TESTS" << std::endl;
    std::cout << "==============================" << std::endl;

    testPass();
    testWrongPass();

    testNick();
    testUser();

    testRegistration();

    testNickWithoutParameter();
    testPassWithoutParameter();
    testWrongPasswordReply();
    testUserWithoutParameters();

    testInvalidNick();
    testPassAfterRegistration();
    testUserAfterRegistration();
    testUnknownCommand();
    testLowercaseCommand();

    testRegistrationWithoutPass();
    testRegistrationWithoutNick();
    testRegistrationWithoutUser();

    //testDuplicateNick();

    std::cout << "==============================" << std::endl;
    std::cout << std::endl;
}