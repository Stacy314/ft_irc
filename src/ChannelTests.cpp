#include "../incl/ChannelTests.hpp"
#include "../incl/Channel.hpp"
#include "../incl/Client.hpp"

#include <iostream>
#include <string>
#include <vector>

/*
 * Tests for the Channel class only.
 *
 * No Server, no sockets, no poll loop — every test builds its own clients
 * and channel in memory. This suite is therefore unaffected by the state of
 * the network layer and can be run at any time.
 */

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
 * Helper: build a client with a nickname
 * -------------------------------------------------------
 */

static Client makeClient(
    int fd,
    const std::string &nick)
{
    Client client(fd, "localhost");

    client.setNickname(nick);
    client.setUsername(nick);
    client.setRegistered(true);

    return client;
}

/*
 * =======================================================
 * MEMBERSHIP
 * =======================================================
 */

static void testFirstMemberBecomesOperator()
{
    Channel channel("#test");
    Client alice = makeClient(1, "alice");
    Client bob = makeClient(2, "bob");

    printTest(
        "addMember: first join succeeds",
        channel.addMember(&alice, "") == SUCCESS
    );

    printTest(
        "addMember: first member is operator",
        channel.isOperator(&alice)
    );

    channel.addMember(&bob, "");

    printTest(
        "addMember: second member is not operator",
        !channel.isOperator(&bob)
    );

    printTest(
        "addMember: both are members",
        channel.isMember(&alice) && channel.isMember(&bob)
    );

    printTest(
        "size reflects both members",
        channel.size() == 2
    );
}

static void testDuplicateJoin()
{
    Channel channel("#test");
    Client alice = makeClient(1, "alice");

    channel.addMember(&alice, "");

    printTest(
        "addMember: joining twice returns ALREADY_MEMBER",
        channel.addMember(&alice, "") == ALREADY_MEMBER
    );

    printTest(
        "addMember: duplicate join does not grow the channel",
        channel.size() == 1
    );
}

static void testRemoveMemberPromotesOldest()
{
    Channel channel("#test");
    Client alice = makeClient(1, "alice");
    Client bob = makeClient(2, "bob");
    Client carol = makeClient(3, "carol");

    channel.addMember(&alice, "");
    channel.addMember(&bob, "");
    channel.addMember(&carol, "");

    printTest(
        "removeMember: removing the only operator succeeds",
        channel.removeMember(&alice) == SUCCESS
    );

    printTest(
        "removeMember: operator passes to the longest-standing member",
        channel.isOperator(&bob)
    );

    printTest(
        "removeMember: the newer member is not promoted",
        !channel.isOperator(&carol)
    );
}

static void testRemoveLastMemberReportsEmpty()
{
    Channel channel("#test");
    Client alice = makeClient(1, "alice");

    channel.addMember(&alice, "");

    printTest(
        "removeMember: last member reports CHANNEL_EMPTY",
        channel.removeMember(&alice) == CHANNEL_EMPTY
    );

    printTest(
        "removeMember: channel is empty afterwards",
        channel.isEmpty()
    );
}

static void testRemoveNonMember()
{
    Channel channel("#test");
    Client alice = makeClient(1, "alice");
    Client stranger = makeClient(9, "stranger");

    channel.addMember(&alice, "");

    printTest(
        "removeMember: removing a non-member is rejected",
        channel.removeMember(&stranger) == NOT_IN_CHANNEL
    );

    printTest(
        "removeMember: rejection leaves the channel untouched",
        channel.size() == 1
    );
}

/*
 * =======================================================
 * OPERATOR PRIVILEGES
 * =======================================================
 */

static void testSetOperatorPermissions()
{
    Channel channel("#test");
    Client alice = makeClient(1, "alice");
    Client bob = makeClient(2, "bob");
    Client carol = makeClient(3, "carol");
    Client stranger = makeClient(9, "stranger");

    channel.addMember(&alice, "");
    channel.addMember(&bob, "");

    printTest(
        "setOperator: a plain member cannot grant operator status",
        channel.setOperator(&bob, &bob, true) == NOT_OPERATOR
    );

    printTest(
        "setOperator: an outsider cannot act on the channel",
        channel.setOperator(&stranger, &bob, true) == NOT_ON_CHANNEL
    );

    printTest(
        "setOperator: target must be on the channel",
        channel.setOperator(&alice, &carol, true) == NOT_IN_CHANNEL
    );

    printTest(
        "setOperator: an operator can promote a member",
        channel.setOperator(&alice, &bob, true) == SUCCESS
    );

    printTest(
        "setOperator: the member is now an operator",
        channel.isOperator(&bob)
    );

    printTest(
        "setOperator: promoting again reports NO_CHANGE",
        channel.setOperator(&alice, &bob, true) == NO_CHANGE
    );

    printTest(
        "setOperator: demotion succeeds",
        channel.setOperator(&alice, &bob, false) == SUCCESS
    );

    printTest(
        "setOperator: the member is no longer an operator",
        !channel.isOperator(&bob)
    );
}

static void testKick()
{
    Channel channel("#test");
    Client alice = makeClient(1, "alice");
    Client bob = makeClient(2, "bob");
    Client carol = makeClient(3, "carol");

    channel.addMember(&alice, "");
    channel.addMember(&bob, "");
    channel.addMember(&carol, "");

    printTest(
        "kickMember: a plain member cannot kick",
        channel.kickMember(&bob, &carol) == NOT_OPERATOR
    );

    printTest(
        "kickMember: the intended victim is still present",
        channel.isMember(&carol)
    );

    printTest(
        "kickMember: an operator can kick",
        channel.kickMember(&alice, &carol) == SUCCESS
    );

    printTest(
        "kickMember: the victim is gone",
        !channel.isMember(&carol)
    );

    printTest(
        "kickMember: kicking someone who is not on the channel is rejected",
        channel.kickMember(&alice, &carol) == NOT_IN_CHANNEL
    );
}

/*
 * =======================================================
 * KEY (+k)
 * =======================================================
 */

static void testKey()
{
    Channel channel("#test");
    Client alice = makeClient(1, "alice");
    Client bob = makeClient(2, "bob");
    Client carol = makeClient(3, "carol");

    channel.addMember(&alice, "");

    printTest(
        "channel starts without a key",
        !channel.hasKey()
    );

    printTest(
        "setKey: operator sets a key",
        channel.setKey(&alice, "secret") == SUCCESS
    );

    printTest(
        "hasKey is true once a key is set",
        channel.hasKey()
    );

    printTest(
        "setKey: setting the same key reports NO_CHANGE",
        channel.setKey(&alice, "secret") == NO_CHANGE
    );

    printTest(
        "addMember: wrong key is rejected",
        channel.addMember(&bob, "wrong") == BAD_KEY
    );

    printTest(
        "addMember: missing key is rejected",
        channel.addMember(&bob, "") == BAD_KEY
    );

    printTest(
        "addMember: correct key is accepted",
        channel.addMember(&bob, "secret") == SUCCESS
    );

    printTest(
        "setKey: an empty key clears the key (-k)",
        channel.setKey(&alice, "") == SUCCESS
    );

    printTest(
        "hasKey is false after clearing",
        !channel.hasKey()
    );

    printTest(
        "addMember: any key is accepted once the key is cleared",
        channel.addMember(&carol, "anything") == SUCCESS
    );
}

/*
 * =======================================================
 * USER LIMIT (+l)
 * =======================================================
 */

static void testUserLimit()
{
    Channel channel("#test");
    Client alice = makeClient(1, "alice");
    Client bob = makeClient(2, "bob");
    Client carol = makeClient(3, "carol");

    channel.addMember(&alice, "");

    printTest(
        "channel is not full without a limit",
        !channel.isFull()
    );

    printTest(
        "setUserLimit: operator sets a limit",
        channel.setUserLimit(&alice, 2) == SUCCESS
    );

    printTest(
        "setUserLimit: setting the same limit reports NO_CHANGE",
        channel.setUserLimit(&alice, 2) == NO_CHANGE
    );

    printTest(
        "addMember: joining below the limit succeeds",
        channel.addMember(&bob, "") == SUCCESS
    );

    printTest(
        "channel is full at the limit",
        channel.isFull()
    );

    printTest(
        "addMember: joining a full channel is rejected",
        channel.addMember(&carol, "") == CHANNEL_FULL
    );

    printTest(
        "setUserLimit: zero removes the limit (-l)",
        channel.setUserLimit(&alice, 0) == SUCCESS
    );

    printTest(
        "addMember: joining succeeds once the limit is removed",
        channel.addMember(&carol, "") == SUCCESS
    );
}

/*
 * =======================================================
 * INVITE ONLY (+i) AND INVITE
 * =======================================================
 */

static void testInviteOnly()
{
    Channel channel("#test");
    Client alice = makeClient(1, "alice");
    Client bob = makeClient(2, "bob");

    channel.addMember(&alice, "");

    printTest(
        "setInviteOnly: operator enables +i",
        channel.setInviteOnly(&alice, true) == SUCCESS
    );

    printTest(
        "setInviteOnly: enabling twice reports NO_CHANGE",
        channel.setInviteOnly(&alice, true) == NO_CHANGE
    );

    printTest(
        "addMember: uninvited client is rejected on an +i channel",
        channel.addMember(&bob, "") == INVITE_ONLY
    );

    printTest(
        "inviteMember: operator invites a client",
        channel.inviteMember(&alice, &bob) == SUCCESS
    );

    printTest(
        "addMember: invited client may join",
        channel.addMember(&bob, "") == SUCCESS
    );
}

static void testInviteIsConsumedOnJoin()
{
    Channel channel("#test");
    Client alice = makeClient(1, "alice");
    Client bob = makeClient(2, "bob");

    channel.addMember(&alice, "");
    channel.setInviteOnly(&alice, true);
    channel.inviteMember(&alice, &bob);
    channel.addMember(&bob, "");
    channel.removeMember(&bob);

    printTest(
        "invite is single use: rejoining after leaving is rejected",
        channel.addMember(&bob, "") == INVITE_ONLY
    );
}

static void testInvitePermissions()
{
    Channel channel("#test");
    Client alice = makeClient(1, "alice");
    Client bob = makeClient(2, "bob");
    Client carol = makeClient(3, "carol");
    Client stranger = makeClient(9, "stranger");

    channel.addMember(&alice, "");
    channel.addMember(&bob, "");

    printTest(
        "inviteMember: an outsider cannot invite",
        channel.inviteMember(&stranger, &carol) == NOT_ON_CHANNEL
    );

    printTest(
        "inviteMember: on an open channel a plain member may invite",
        channel.inviteMember(&bob, &carol) == SUCCESS
    );

    channel.setInviteOnly(&alice, true);

    printTest(
        "inviteMember: on an +i channel a plain member may not invite",
        channel.inviteMember(&bob, &carol) == NOT_OPERATOR
    );

    printTest(
        "inviteMember: inviting an existing member is rejected",
        channel.inviteMember(&alice, &bob) == ALREADY_MEMBER
    );
}

/*
 * =======================================================
 * TOPIC (+t)
 * =======================================================
 */

static void testTopic()
{
    Channel channel("#test");
    Client alice = makeClient(1, "alice");
    Client bob = makeClient(2, "bob");
    Client stranger = makeClient(9, "stranger");

    channel.addMember(&alice, "");
    channel.addMember(&bob, "");

    printTest(
        "channel starts without a topic",
        !channel.hasTopic()
    );

    printTest(
        "setTopic: an outsider cannot set the topic",
        channel.setTopic(&stranger, "hello") == NOT_ON_CHANNEL
    );

    printTest(
        "setTopic: without +t a plain member may set the topic",
        channel.setTopic(&bob, "hello") == SUCCESS
    );

    printTest(
        "hasTopic is true once a topic is set",
        channel.hasTopic()
    );

    printTest(
        "getTopic returns what was set",
        channel.getTopic() == "hello"
    );

    printTest(
        "setProtectedTopic: operator enables +t",
        channel.setProtectedTopic(&alice, true) == SUCCESS
    );

    printTest(
        "setProtectedTopic: enabling twice reports NO_CHANGE",
        channel.setProtectedTopic(&alice, true) == NO_CHANGE
    );

    printTest(
        "setTopic: with +t a plain member may not set the topic",
        channel.setTopic(&bob, "nope") == NOT_OPERATOR
    );

    printTest(
        "setTopic: the topic is unchanged after a rejected attempt",
        channel.getTopic() == "hello"
    );

    printTest(
        "setTopic: with +t an operator may set the topic",
        channel.setTopic(&alice, "new topic") == SUCCESS
    );

    printTest(
        "setTopic: an empty topic clears it",
        channel.setTopic(&alice, "") == SUCCESS
    );

    printTest(
        "hasTopic is false after clearing",
        !channel.hasTopic()
    );
}

/*
 * =======================================================
 * ACCESSORS USED BY THE DISPATCHER
 * =======================================================
 */

static void testAccessors()
{
    Channel channel("#general");
    Client alice = makeClient(1, "alice");
    Client bob = makeClient(2, "bob");

    channel.addMember(&alice, "");
    channel.addMember(&bob, "");

    printTest(
        "getName returns the channel name",
        channel.getName() == "#general"
    );

    std::vector<Client*> members = channel.getMembers();

    printTest(
        "getMembers returns every member",
        members.size() == 2
    );

    bool foundAlice = false;
    bool foundBob = false;

    for (size_t i = 0; i < members.size(); ++i)
    {
        if (members[i] == &alice)
            foundAlice = true;
        if (members[i] == &bob)
            foundBob = true;
    }

    printTest(
        "getMembers contains both clients",
        foundAlice && foundBob
    );

    members.clear();

    printTest(
        "getMembers returns a copy, not a view into the channel",
        channel.size() == 2
    );
}

/*
 * =======================================================
 * ENTRY POINT
 * =======================================================
 */

void runChannelTests()
{
    g_passed = 0;
    g_failed = 0;

    std::cout << "=== Channel tests ===" << std::endl;

    testFirstMemberBecomesOperator();
    testDuplicateJoin();
    testRemoveMemberPromotesOldest();
    testRemoveLastMemberReportsEmpty();
    testRemoveNonMember();

    testSetOperatorPermissions();
    testKick();

    testKey();
    testUserLimit();

    testInviteOnly();
    testInviteIsConsumedOnJoin();
    testInvitePermissions();

    testTopic();

    testAccessors();

    std::cout
        << "--- passed: " << g_passed
        << ", failed: " << g_failed
        << " ---" << std::endl;
}
