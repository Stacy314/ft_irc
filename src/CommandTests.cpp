#include "../incl/CommandTests.hpp"
#include "../incl/CommandHandler.hpp"
#include "../incl/Command.hpp"
#include "../incl/Parser.hpp"
#include "../incl/Client.hpp"
#include "../incl/Server.hpp"
#include "../incl/Channel.hpp"

#include <iostream>
#include <string>

static int g_passed = 0;
static int g_failed = 0;

static void printTest(
    const std::string &name,
    bool result)
{
    if (result)
    {
        std::cout << "[OK]   " << name << std::endl;
        ++g_passed;
    }
    else
    {
        std::cout << "[FAIL] " << name << std::endl;
        ++g_failed;
    }
}

/*
 * -------------------------------------------------------
 * Helper: register one client
 * -------------------------------------------------------
 */

static void registerClient(
    CommandHandler &handler,
    Client &client,
    const std::string &nick,
    const std::string &user,
    const std::string &realname)
{
    handler.execute(
        client,
        Parser::parse("PASS secret")
    );

    handler.execute(
        client,
        Parser::parse("NICK " + nick)
    );

    handler.execute(
        client,
        Parser::parse(
            "USER "
            + user
            + " 0 * :"
            + realname
        )
    );
}

/*
 * =======================================================
 * PASS TESTS
 * =======================================================
 */

static void testPass()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse("PASS secret")
    );

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

    handler.execute(
        client,
        Parser::parse("PASS wrong")
    );

    printTest(
        "PASS wrong password",
        !client.isPasswordAccepted()
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
        client.outbuf.find("461")
            != std::string::npos
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
        client.outbuf.find("464")
            != std::string::npos
    );
}

/*
 * =======================================================
 * NICK TESTS
 * =======================================================
 */

static void testNick()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse("NICK bob")
    );

    printTest(
        "NICK sets nickname",
        client.getNickname() == "bob"
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
        client.outbuf.find("431")
            != std::string::npos
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
        client.outbuf.find("432")
            != std::string::npos
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

/*
 * =======================================================
 * USER TESTS
 * =======================================================
 */

static void testUser()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse(
            "USER bob 0 * :Bob Smith"
        )
    );

    printTest(
        "USER sets username",
        client.getUsername() == "bob"
    );

    printTest(
        "USER was received",
        client.isUserReceived()
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
        client.outbuf.find("461")
            != std::string::npos
    );
}

/*
 * =======================================================
 * REGISTRATION TESTS
 * =======================================================
 */

static void testRegistration()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    registerClient(
        handler,
        client,
        "bob",
        "bob",
        "Bob Smith"
    );

    printTest(
        "Client registered",
        client.isRegistered()
    );

    printTest(
        "001 Welcome generated",
        client.outbuf.find("001")
            != std::string::npos
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
        Parser::parse(
            "USER bob 0 * :Bob Smith"
        )
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
        Parser::parse(
            "USER bob 0 * :Bob Smith"
        )
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

static void testPassAfterRegistration()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    registerClient(
        handler,
        client,
        "bob",
        "bob",
        "Bob Smith"
    );

    client.outbuf.clear();

    handler.execute(
        client,
        Parser::parse("PASS secret")
    );

    printTest(
        "PASS after registration -> 462",
        client.outbuf.find("462")
            != std::string::npos
    );
}

static void testUserAfterRegistration()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    registerClient(
        handler,
        client,
        "bob",
        "bob",
        "Bob Smith"
    );

    client.outbuf.clear();

    handler.execute(
        client,
        Parser::parse(
            "USER alice 0 * :Alice Smith"
        )
    );

    printTest(
        "USER after registration -> 462",
        client.outbuf.find("462")
            != std::string::npos
    );
}

/*
 * =======================================================
 * COMMAND DISPATCH TEST
 * =======================================================
 */

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
        client.outbuf.find("421")
            != std::string::npos
    );
}

/*
 * =======================================================
 * JOIN TESTS
 * =======================================================
 */

static void testJoinCreatesAndAddsMember()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    registerClient(
        handler,
        client,
        "bob",
        "bob",
        "Bob Smith"
    );

    handler.execute(
        client,
        Parser::parse("JOIN #general")
    );

    Channel *channel =
        server.findChannel("#general");

    printTest(
        "JOIN creates channel",
        channel != NULL
    );

    printTest(
        "JOIN adds client to channel",
        channel != NULL
        && channel->isMember(&client)
    );
}

static void testJoinTwice()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    registerClient(
        handler,
        client,
        "bob",
        "bob",
        "Bob Smith"
    );

    handler.execute(
        client,
        Parser::parse("JOIN #general")
    );

    Channel *channel =
        server.findChannel("#general");

    std::size_t sizeBefore = 0;

    if (channel != NULL)
        sizeBefore = channel->size();

    handler.execute(
        client,
        Parser::parse("JOIN #general")
    );

    printTest(
        "JOIN twice doesn't duplicate member",
        channel != NULL
        && channel->size() == sizeBefore
    );
}

static void testJoinWithoutRegistration()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse("JOIN #general")
    );

    printTest(
        "JOIN without registration -> 451",
        client.outbuf.find("451")
            != std::string::npos
    );
}

static void testJoinWithoutChannel()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    registerClient(
        handler,
        client,
        "bob",
        "bob",
        "Bob Smith"
    );

    client.outbuf.clear();

    handler.execute(
        client,
        Parser::parse("JOIN")
    );

    printTest(
        "JOIN without channel -> 461",
        client.outbuf.find("461")
            != std::string::npos
    );
}

static void testJoinBadKey()
{
    Server server(6667, "secret");
    CommandHandler handler(server);

    Client owner;
    Client guest;

    registerClient(
        handler,
        owner,
        "owner",
        "owner",
        "Channel Owner"
    );

    /*
     * Guest реєструємо вручну, щоб не залежати
     * від Server client storage в цьому тесті.
     */
    guest.setPasswordAccepted(true);
    guest.setNickname("alice");
    guest.setUsername("alice");
    guest.setRealname("Alice");
    guest.setRegistered(true);

    Channel &channel =
        server.createChannel("#private");

    ChannelResult result =
        channel.addMember(&owner, "");

    printTest(
        "Owner joins channel",
        result == SUCCESS
    );

    result =
        channel.setKey("correct", &owner);

    printTest(
        "Operator sets channel key",
        result == SUCCESS
        || result == NO_CHANGE
    );

    guest.outbuf.clear();

    handler.execute(
        guest,
        Parser::parse(
            "JOIN #private wrong"
        )
    );

    printTest(
        "JOIN wrong key -> 475",
        guest.outbuf.find("475")
            != std::string::npos
    );

    printTest(
        "Wrong key doesn't add member",
        !channel.isMember(&guest)
    );
}

/*
 * =======================================================
 * PRIVMSG TESTS
 * =======================================================
 */

static void testPrivmsgWithoutRegistration()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    handler.execute(
        client,
        Parser::parse(
            "PRIVMSG bob :hello"
        )
    );

    printTest(
        "PRIVMSG without registration -> 451",
        client.outbuf.find("451")
            != std::string::npos
    );
}

static void testPrivmsgWithoutTarget()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    registerClient(
        handler,
        client,
        "bob",
        "bob",
        "Bob Smith"
    );

    client.outbuf.clear();

    handler.execute(
        client,
        Parser::parse("PRIVMSG")
    );

    printTest(
        "PRIVMSG without target -> 411",
        client.outbuf.find("411")
            != std::string::npos
    );
}

static void testPrivmsgWithoutText()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    registerClient(
        handler,
        client,
        "bob",
        "bob",
        "Bob Smith"
    );

    client.outbuf.clear();

    handler.execute(
        client,
        Parser::parse("PRIVMSG alice")
    );

    printTest(
        "PRIVMSG without text -> 412",
        client.outbuf.find("412")
            != std::string::npos
    );
}

static void testPrivmsgUnknownChannel()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    registerClient(
        handler,
        client,
        "bob",
        "bob",
        "Bob Smith"
    );

    client.outbuf.clear();

    handler.execute(
        client,
        Parser::parse(
            "PRIVMSG #unknown :hello"
        )
    );

    printTest(
        "PRIVMSG unknown channel -> 403",
        client.outbuf.find("403")
            != std::string::npos
    );
}

static void testPrivmsgNotMember()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    registerClient(
        handler,
        client,
        "bob",
        "bob",
        "Bob Smith"
    );

    server.createChannel("#general");

    client.outbuf.clear();

    handler.execute(
        client,
        Parser::parse(
            "PRIVMSG #general :hello"
        )
    );

    printTest(
        "PRIVMSG when not member -> 404",
        client.outbuf.find("404")
            != std::string::npos
    );
}

static void testPrivmsgUnknownNick()
{
    Server server(6667, "secret");
    CommandHandler handler(server);
    Client client;

    registerClient(
        handler,
        client,
        "bob",
        "bob",
        "Bob Smith"
    );

    client.outbuf.clear();

    handler.execute(
        client,
        Parser::parse(
            "PRIVMSG nobody :hello"
        )
    );

    printTest(
        "PRIVMSG unknown nick -> 401",
        client.outbuf.find("401")
            != std::string::npos
    );
}

/*
 * =======================================================
 * MAIN TEST RUNNER
 * =======================================================
 */

void runCommandTests()
{
    g_passed = 0;
    g_failed = 0;

    std::cout << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "          COMMAND TESTS" << std::endl;
    std::cout << "==================================" << std::endl;

    std::cout << std::endl;
    std::cout << "--- PASS ---" << std::endl;

    testPass();
    testWrongPass();
    testPassWithoutParameter();
    testWrongPasswordReply();

    std::cout << std::endl;
    std::cout << "--- NICK ---" << std::endl;

    testNick();
    testNickWithoutParameter();
    testInvalidNick();
    testLowercaseCommand();

    std::cout << std::endl;
    std::cout << "--- USER ---" << std::endl;

    testUser();
    testUserWithoutParameters();

    std::cout << std::endl;
    std::cout << "--- REGISTRATION ---" << std::endl;

    testRegistration();
    testRegistrationWithoutPass();
    testRegistrationWithoutNick();
    testRegistrationWithoutUser();
    testPassAfterRegistration();
    testUserAfterRegistration();

    std::cout << std::endl;
    std::cout << "--- DISPATCH ---" << std::endl;

    testUnknownCommand();

    std::cout << std::endl;
    std::cout << "--- JOIN ---" << std::endl;

    testJoinCreatesAndAddsMember();
    testJoinTwice();
    testJoinWithoutRegistration();
    testJoinWithoutChannel();
    testJoinBadKey();

    std::cout << std::endl;
    std::cout << "--- PRIVMSG ---" << std::endl;

    testPrivmsgWithoutRegistration();
    testPrivmsgWithoutTarget();
    testPrivmsgWithoutText();
    testPrivmsgUnknownChannel();
    testPrivmsgNotMember();
    testPrivmsgUnknownNick();

    std::cout << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "PASSED: " << g_passed << std::endl;
    std::cout << "FAILED: " << g_failed << std::endl;
    std::cout << "==================================" << std::endl;
}