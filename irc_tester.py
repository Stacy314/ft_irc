#!/usr/bin/env python3

import socket
import subprocess
import time
import os
import sys
import signal

HOST = "127.0.0.1"
PORT = 1235
PASSWORD = "a"

SERVER_EXEC = "./ircserv"
TIMEOUT = 0.4

passed = 0
failed = 0
server_process = None


class IRCClient:
    def __init__(self, name):
        self.name = name
        self.sock = None

    def connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(TIMEOUT)
        self.sock.connect((HOST, PORT))

        time.sleep(0.05)
        self.read()

    def send(self, command):
        print("[%s ->] %s" % (self.name, command))
        self.sock.sendall((command + "\r\n").encode())
        time.sleep(0.05)

    def read(self):
        if self.sock is None:
            return ""

        result = ""

        while True:
            try:
                data = self.sock.recv(4096)

                if not data:
                    break

                result += data.decode(errors="replace")

            except socket.timeout:
                break

            except OSError:
                break

        if result:
            print("[%s <-]" % self.name)

            for line in result.splitlines():
                print("   " + line)

        return result

    def clear(self):
        self.read()

    def register(self, nick):
        self.send("PASS " + PASSWORD)
        self.send("NICK " + nick)
        self.send("USER " + nick + " 0 * :" + nick)

        return self.read()

    def close(self):
        if self.sock is not None:
            try:
                self.sock.close()
            except OSError:
                pass

            self.sock = None


def ok(name):
    global passed
    passed += 1
    print("\033[92m[OK]\033[0m   " + name)


def fail(name, details=""):
    global failed
    failed += 1

    print("\033[91m[FAIL]\033[0m " + name)

    if details:
        print(details)


def check(name, condition, details=""):
    if condition:
        ok(name)
    else:
        fail(name, details)


def section(name):
    print()
    print("=" * 60)
    print(name)
    print("=" * 60)


# ============================================================
# SERVER
# ============================================================

def start_server():
    global server_process

    print("Starting server...")

    server_process = subprocess.Popen(
        [
            SERVER_EXEC,
            str(PORT),
            PASSWORD
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )

    for _ in range(30):
        time.sleep(0.1)

        try:
            sock = socket.create_connection(
                (HOST, PORT),
                timeout=0.1
            )

            sock.close()

            print("Server started.")
            return True

        except OSError:
            pass

    print("ERROR: server did not start.")

    return False


def stop_server():
    global server_process

    if server_process is None:
        return

    print()
    print("Stopping server...")

    if server_process.poll() is None:
        server_process.terminate()

        try:
            server_process.wait(timeout=2)

        except subprocess.TimeoutExpired:
            server_process.kill()
            server_process.wait()

    print("Server stopped.")


# def server_alive():
#     return (
#         server_process is not None
#         and server_process.poll() is None
#     )
def server_alive():
    if server_process is None:
        print("DEBUG: server_process is None")
        return False

    status = server_process.poll()

    print("DEBUG: server poll =", status)

    return status is None


# ============================================================
# REGISTRATION
# ============================================================

def test_registration():
    section("REGISTRATION")

    alice = IRCClient("alice")

    try:
        alice.connect()

        response = alice.register("alice")

        check(
            "Registration -> 001",
            " 001 " in response,
            response
        )

        check(
            "Registration -> 002",
            " 002 " in response,
            response
        )

        check(
            "Registration -> 003",
            " 003 " in response,
            response
        )

        check(
            "Registration -> 004",
            " 004 " in response,
            response
        )

    finally:
        alice.close()


# ============================================================
# WRONG PASSWORD
# ============================================================

def test_wrong_password():
    section("PASS")

    client = IRCClient("wrong-pass")

    try:
        client.connect()

        client.send("PASS wrongpassword")

        response = client.read()

        check(
            "Wrong PASS -> 464",
            " 464 " in response,
            response
        )

    finally:
        client.close()


# ============================================================
# DUPLICATE NICK
# ============================================================

def test_duplicate_nick():
    section("NICK")

    alice = IRCClient("alice")
    bob = IRCClient("bob")

    try:
        alice.connect()
        bob.connect()

        alice.register("taken")
        alice.clear()

        bob.send("PASS " + PASSWORD)
        bob.send("NICK taken")

        response = bob.read()

        check(
            "Duplicate NICK -> 433",
            " 433 " in response,
            response
        )

    finally:
        alice.close()
        bob.close()


# ============================================================
# JOIN + CHANNEL MESSAGES
# ============================================================

def test_channel():
    section("CHANNEL / PRIVMSG")

    alice = IRCClient("alice")
    bob = IRCClient("bob")
    charlie = IRCClient("charlie")

    try:
        alice.connect()
        bob.connect()
        charlie.connect()

        alice.register("alice")
        bob.register("bob")
        charlie.register("charlie")

        alice.clear()
        bob.clear()
        charlie.clear()

        alice.send("JOIN #test")
        alice.read()

        bob.send("JOIN #test")
        bob.read()

        charlie.send("JOIN #test")
        charlie.read()

        alice.clear()
        bob.clear()
        charlie.clear()

        alice.send(
            "PRIVMSG #test :hello everybody"
        )

        time.sleep(0.1)

        alice_response = alice.read()
        bob_response = bob.read()
        charlie_response = charlie.read()

        check(
            "Bob receives channel PRIVMSG",
            "hello everybody" in bob_response,
            bob_response
        )

        check(
            "Charlie receives channel PRIVMSG",
            "hello everybody" in charlie_response,
            charlie_response
        )

        check(
            "Sender does not receive own PRIVMSG",
            "hello everybody" not in alice_response,
            alice_response
        )

    finally:
        alice.close()
        bob.close()
        charlie.close()


# ============================================================
# PRIVATE MESSAGE
# ============================================================

def test_private_message():
    section("PRIVATE PRIVMSG")

    alice = IRCClient("alice")
    bob = IRCClient("bob")

    try:
        alice.connect()
        bob.connect()

        alice.register("alice")
        bob.register("bob")

        alice.clear()
        bob.clear()

        alice.send(
            "PRIVMSG bob :hello bob"
        )

        response = bob.read()

        check(
            "Private message arrives",
            "hello bob" in response,
            response
        )

    finally:
        alice.close()
        bob.close()


# ============================================================
# PART
# ============================================================

def test_part():
    section("PART")

    alice = IRCClient("alice")
    bob = IRCClient("bob")

    try:
        alice.connect()
        bob.connect()

        alice.register("alice")
        bob.register("bob")

        alice.clear()
        bob.clear()

        alice.send("JOIN #part")
        alice.read()

        bob.send("JOIN #part")
        bob.read()

        alice.clear()
        bob.clear()

        alice.send(
            "PART #part :bye"
        )

        time.sleep(0.1)

        alice_response = alice.read()
        bob_response = bob.read()

        check(
            "Alice receives PART",
            "PART #part" in alice_response,
            alice_response
        )

        check(
            "Bob receives Alice PART",
            "PART #part" in bob_response,
            bob_response
        )

        alice.send(
            "PRIVMSG #part :test"
        )

        response = alice.read()

        check(
            "PARTed client cannot send",
            " 404 " in response
            or " 403 " in response,
            response
        )

    finally:
        alice.close()
        bob.close()


# ============================================================
# JOIN 0
# ============================================================

def test_join_zero():
    section("JOIN 0")

    alice = IRCClient("alice")

    try:
        alice.connect()
        alice.register("alice")

        alice.clear()

        alice.send("JOIN #one")
        alice.read()

        alice.send("JOIN #two")
        alice.read()

        alice.clear()

        alice.send("JOIN 0")

        response = alice.read()

        check(
            "JOIN 0 leaves #one",
            "PART #one" in response,
            response
        )

        check(
            "JOIN 0 leaves #two",
            "PART #two" in response,
            response
        )

        alice.send("JOIN #new")

        response = alice.read()

        check(
            "Connection stays alive after JOIN 0",
            "JOIN #new" in response
            or "JOIN :#new" in response,
            response
        )

    finally:
        alice.close()


# ============================================================
# KICK
# ============================================================

def test_kick():
    section("KICK")

    alice = IRCClient("alice")
    bob = IRCClient("bob")

    try:
        alice.connect()
        bob.connect()

        alice.register("alice")
        bob.register("bob")

        alice.clear()
        bob.clear()

        alice.send("JOIN #kick")
        alice.read()

        bob.send("JOIN #kick")
        bob.read()

        alice.clear()
        bob.clear()

        alice.send(
            "KICK #kick bob :bye"
        )

        time.sleep(0.1)

        a = alice.read()
        b = bob.read()

        check(
            "Operator can KICK",
            "KICK #kick bob" in a
            or "KICK #kick bob" in b,
            a + b
        )

        bob.send(
            "PRIVMSG #kick :hello"
        )

        response = bob.read()

        check(
            "Kicked client cannot send",
            " 404 " in response,
            response
        )

    finally:
        alice.close()
        bob.close()


# ============================================================
# MODE +k
# ============================================================

def test_mode_key():
    section("MODE +k")

    alice = IRCClient("alice")
    bob = IRCClient("bob")

    try:
        alice.connect()
        bob.connect()

        alice.register("alice")
        bob.register("bob")

        alice.send("JOIN #key")
        alice.read()

        alice.send(
            "MODE #key +k secret"
        )

        alice.read()

        bob.clear()

        bob.send(
            "JOIN #key wrong"
        )

        response = bob.read()

        check(
            "Wrong key -> 475",
            " 475 " in response,
            response
        )

        bob.send(
            "JOIN #key secret"
        )

        response = bob.read()

        check(
            "Correct key allows JOIN",
            "JOIN #key" in response
            or "JOIN :#key" in response,
            response
        )

    finally:
        alice.close()
        bob.close()


# ============================================================
# MODE +i / INVITE
# ============================================================

def test_invite():
    section("MODE +i / INVITE")

    alice = IRCClient("alice")
    bob = IRCClient("bob")

    try:
        alice.connect()
        bob.connect()

        alice.register("alice")
        bob.register("bob")

        alice.send(
            "JOIN #invite"
        )

        alice.read()

        alice.send(
            "MODE #invite +i"
        )

        alice.read()

        bob.clear()

        bob.send(
            "JOIN #invite"
        )

        response = bob.read()

        check(
            "Invite-only -> 473",
            " 473 " in response,
            response
        )

        alice.send(
            "INVITE bob #invite"
        )

        alice.read()
        bob.read()

        bob.send(
            "JOIN #invite"
        )

        response = bob.read()

        check(
            "INVITE allows JOIN",
            "JOIN #invite" in response
            or "JOIN :#invite" in response,
            response
        )

    finally:
        alice.close()
        bob.close()


# ============================================================
# PARTIAL TCP
# ============================================================

def test_partial_command():
    section("PARTIAL TCP COMMAND")

    client = IRCClient("partial")

    try:
        client.connect()

        client.sock.sendall(b"PA")
        time.sleep(0.05)

        client.sock.sendall(b"SS ")
        time.sleep(0.05)

        client.sock.sendall(
            PASSWORD.encode()
        )

        client.sock.sendall(b"\r\n")

        client.sock.sendall(
            b"NICK par"
        )

        time.sleep(0.05)

        client.sock.sendall(
            b"tial\r\n"
        )

        client.sock.sendall(
            b"USER partial 0 * :Partial User\r\n"
        )

        time.sleep(0.2)

        response = client.read()

        check(
            "Partial command reconstructed",
            " 001 partial " in response,
            response
        )

    finally:
        client.close()


# ============================================================
# SERVER SURVIVES BAD CLIENT
# ============================================================

def test_abrupt_disconnect():
    section("ABRUPT DISCONNECT")

    bad = IRCClient("bad")

    bad.connect()

    bad.sock.sendall(
        b"PRIVMSG #test :half"
    )

    # Kill connection without CRLF / QUIT
    bad.close()

    time.sleep(0.2)

    check(
        "Server survives killed client",
        server_alive()
    )

    good = IRCClient("good")

    try:
        good.connect()

        response = good.register("survivor")

        check(
            "New client works after bad disconnect",
            " 001 survivor " in response,
            response
        )

    finally:
        good.close()


# ============================================================
# QUIT
# ============================================================

def test_quit():
    section("QUIT")

    client = IRCClient("quit")

    try:
        client.connect()
        client.register("quitter")

        client.clear()

        client.send(
            "QUIT :bye"
        )

        time.sleep(0.1)

        client.read()

        disconnected = False

        try:
            client.sock.sendall(
                b"PING hi\r\n"
            )

            data = client.sock.recv(1024)

            if not data:
                disconnected = True

        except (
            BrokenPipeError,
            ConnectionResetError,
            OSError
        ):
            disconnected = True

        check(
            "QUIT closes connection",
            disconnected
        )

        check(
            "Server survives QUIT",
            server_alive()
        )

    finally:
        client.close()

# ============================================================
# EDGE CASE: COMMANDS BEFORE REGISTRATION
# ============================================================

def test_commands_before_registration():
    section("EDGE: COMMANDS BEFORE REGISTRATION")

    client = IRCClient("unregistered")

    try:
        client.connect()
        client.clear()

        commands = [
            "JOIN #test",
            "PRIVMSG somebody :hello",
            "PART #test"
        ]

        for command in commands:
            client.send(command)
            response = client.read()

            check(
                command + " before registration -> 451",
                " 451 " in response,
                response
            )

    finally:
        client.close()


# ============================================================
# EDGE CASE: MISSING PARAMETERS
# ============================================================

def test_missing_parameters():
    section("EDGE: MISSING PARAMETERS")

    client = IRCClient("missing")

    try:
        client.connect()

        client.send("PASS")
        response = client.read()

        check(
            "PASS without password -> 461",
            " 461 " in response,
            response
        )

        client.send("NICK")
        response = client.read()

        check(
            "NICK without nickname -> 431",
            " 431 " in response,
            response
        )

        client.send("PASS " + PASSWORD)
        client.send("NICK missing")
        client.send("USER missing 0 * :Missing")
        client.read()

        client.send("JOIN")
        response = client.read()

        check(
            "JOIN without channel -> 461",
            " 461 " in response,
            response
        )

        client.send("PRIVMSG")
        response = client.read()

        check(
            "PRIVMSG without recipient -> 411",
            " 411 " in response,
            response
        )

        client.send("PRIVMSG nobody")
        response = client.read()

        check(
            "PRIVMSG without text -> 412",
            " 412 " in response,
            response
        )

        client.send("PART")
        response = client.read()

        check(
            "PART without channel -> 461",
            " 461 " in response,
            response
        )

    finally:
        client.close()


# ============================================================
# EDGE CASE: MULTIPLE COMMANDS IN ONE TCP PACKET
# ============================================================

def test_multiple_commands_one_packet():
    section("EDGE: MULTIPLE COMMANDS IN ONE PACKET")

    client = IRCClient("multi-packet")

    try:
        client.connect()
        client.clear()

        packet = (
            "PASS " + PASSWORD + "\r\n"
            "NICK multipacket\r\n"
            "USER multipacket 0 * :Multi Packet\r\n"
        )

        client.sock.sendall(packet.encode())

        time.sleep(0.2)

        response = client.read()

        check(
            "Several IRC commands in one send()",
            " 001 multipacket " in response,
            response
        )

    finally:
        client.close()


# ============================================================
# EDGE CASE: INCOMPLETE COMMAND MUST NOT BLOCK SERVER
# ============================================================

def test_incomplete_client_does_not_block():
    section("EDGE: INCOMPLETE CLIENT DOES NOT BLOCK")

    slow = IRCClient("slow")
    fast = IRCClient("fast")

    try:
        slow.connect()

        # Intentionally no CRLF.
        slow.sock.sendall(b"PASS ")

        time.sleep(0.1)

        fast.connect()

        response = fast.register("fastclient")

        check(
            "Second client works while first command is incomplete",
            " 001 fastclient " in response,
            response
        )

        check(
            "Server alive with incomplete command",
            server_alive()
        )

    finally:
        slow.close()
        fast.close()


# ============================================================
# EDGE CASE: DISCONNECT IN MIDDLE OF COMMAND
# ============================================================

def test_disconnect_mid_command():
    section("EDGE: DISCONNECT MID COMMAND")

    broken = IRCClient("broken")

    broken.connect()

    broken.sock.sendall(
        b"PRIVMSG #whatever :this command never finishes"
    )

    broken.close()

    time.sleep(0.2)

    check(
        "Server survives disconnect during incomplete command",
        server_alive()
    )

    client = IRCClient("after-broken")

    try:
        client.connect()

        response = client.register("afterbroken")

        check(
            "Server accepts clients after broken connection",
            " 001 afterbroken " in response,
            response
        )

    finally:
        client.close()


# ============================================================
# EDGE CASE: CASE-INSENSITIVE NICKNAME
# ============================================================

def test_case_insensitive_nick():
    section("EDGE: CASE INSENSITIVE NICK")

    alice = IRCClient("alice")
    bob = IRCClient("bob")

    try:
        alice.connect()
        bob.connect()

        alice.register("Alice")
        alice.clear()

        bob.send("PASS " + PASSWORD)
        bob.send("NICK ALICE")

        response = bob.read()

        check(
            "Alice and ALICE cannot coexist -> 433",
            " 433 " in response,
            response
        )

    finally:
        alice.close()
        bob.close()


# ============================================================
# EDGE CASE: DUPLICATE JOIN
# ============================================================

def test_duplicate_join():
    section("EDGE: DUPLICATE JOIN")

    alice = IRCClient("alice")
    bob = IRCClient("bob")

    try:
        alice.connect()
        bob.connect()

        alice.register("alice")
        bob.register("bob")

        alice.clear()
        bob.clear()

        alice.send("JOIN #duplicate")
        alice.read()

        # Alice joins again.
        alice.send("JOIN #duplicate")
        duplicate_response = alice.read()

        bob.send("JOIN #duplicate")
        bob.read()

        alice.clear()
        bob.clear()

        bob.send(
            "PRIVMSG #duplicate :only once please"
        )

        time.sleep(0.1)

        response = alice.read()

        # Count the text. Alice should only receive it once.
        count = response.count("only once please")

        check(
            "Duplicate JOIN does not duplicate membership",
            count == 1,
            "Message count = %d\n%s" % (
                count,
                response
            )
        )

        # Silence for second JOIN is acceptable.
        check(
            "Server survives duplicate JOIN",
            server_alive(),
            duplicate_response
        )

    finally:
        alice.close()
        bob.close()


# ============================================================
# EDGE CASE: PART NON-EXISTENT CHANNEL
# ============================================================

def test_part_nonexistent_channel():
    section("EDGE: PART NON-EXISTENT CHANNEL")

    client = IRCClient("alice")

    try:
        client.connect()
        client.register("alice")
        client.clear()

        client.send("PART #doesnotexist")

        response = client.read()

        check(
            "PART nonexistent channel -> 403",
            " 403 " in response,
            response
        )

    finally:
        client.close()


# ============================================================
# EDGE CASE: PART CHANNEL CLIENT IS NOT ON
# ============================================================

def test_part_not_member():
    section("EDGE: PART NOT MEMBER")

    alice = IRCClient("alice")
    bob = IRCClient("bob")

    try:
        alice.connect()
        bob.connect()

        alice.register("alice")
        bob.register("bob")

        alice.send("JOIN #membership")
        alice.read()

        bob.clear()

        bob.send("PART #membership")

        response = bob.read()

        check(
            "PART without membership -> 442",
            " 442 " in response,
            response
        )

    finally:
        alice.close()
        bob.close()


# ============================================================
# EDGE CASE: PRIVMSG UNKNOWN NICK
# ============================================================

def test_privmsg_unknown_nick():
    section("EDGE: PRIVMSG UNKNOWN NICK")

    client = IRCClient("alice")

    try:
        client.connect()
        client.register("alice")
        client.clear()

        client.send(
            "PRIVMSG definitely_not_here :hello"
        )

        response = client.read()

        check(
            "PRIVMSG unknown nickname -> 401",
            " 401 " in response,
            response
        )

    finally:
        client.close()


# ============================================================
# EDGE CASE: PRIVMSG UNKNOWN CHANNEL
# ============================================================

def test_privmsg_unknown_channel():
    section("EDGE: PRIVMSG UNKNOWN CHANNEL")

    client = IRCClient("alice")

    try:
        client.connect()
        client.register("alice")
        client.clear()

        client.send(
            "PRIVMSG #doesnotexist :hello"
        )

        response = client.read()

        check(
            "PRIVMSG unknown channel -> 403",
            " 403 " in response,
            response
        )

    finally:
        client.close()


# ============================================================
# EDGE CASE: CHANNEL MESSAGE AFTER PART
# ============================================================

def test_message_after_part():
    section("EDGE: MESSAGE AFTER PART")

    alice = IRCClient("alice")
    bob = IRCClient("bob")

    try:
        alice.connect()
        bob.connect()

        alice.register("alice")
        bob.register("bob")

        alice.send("JOIN #leave")
        alice.read()

        bob.send("JOIN #leave")
        bob.read()

        alice.clear()
        bob.clear()

        alice.send("PART #leave :gone")
        alice.read()
        bob.read()

        alice.send(
            "PRIVMSG #leave :I should not be able to do this"
        )

        response = alice.read()

        check(
            "Client cannot PRIVMSG after PART -> 404",
            " 404 " in response,
            response
        )

    finally:
        alice.close()
        bob.close()


# ============================================================
# EDGE CASE: NICK CHANGE
# ============================================================

def test_nick_change():
    section("EDGE: NICK CHANGE")

    alice = IRCClient("alice")
    bob = IRCClient("bob")

    try:
        alice.connect()
        bob.connect()

        alice.register("alice")
        bob.register("bob")

        alice.send("JOIN #nicktest")
        alice.read()

        bob.send("JOIN #nicktest")
        bob.read()

        alice.clear()
        bob.clear()

        alice.send("NICK alice2")

        time.sleep(0.1)

        alice_response = alice.read()
        bob_response = bob.read()

        check(
            "Other channel member sees NICK change",
            "NICK" in bob_response
            and "alice2" in bob_response,
            bob_response
        )

        bob.clear()

        bob.send(
            "PRIVMSG alice2 :new nickname works"
        )

        response = alice.read()

        check(
            "New nickname can receive PRIVMSG",
            "new nickname works" in response,
            response
        )

    finally:
        alice.close()
        bob.close()


# ============================================================
# EDGE CASE: QUIT MUST BE BROADCAST
# ============================================================

def test_quit_broadcast():
    section("EDGE: QUIT BROADCAST")

    alice = IRCClient("alice")
    bob = IRCClient("bob")

    try:
        alice.connect()
        bob.connect()

        alice.register("alice")
        bob.register("bob")

        alice.send("JOIN #quit-test")
        alice.read()

        bob.send("JOIN #quit-test")
        bob.read()

        alice.clear()
        bob.clear()

        alice.send("QUIT :goodbye everybody")

        time.sleep(0.2)

        bob_response = bob.read()

        check(
            "Channel member receives QUIT",
            "QUIT" in bob_response
            and "goodbye everybody" in bob_response,
            bob_response
        )

        check(
            "Server survives user QUIT",
            server_alive()
        )

    finally:
        alice.close()
        bob.close()


# ============================================================
# EDGE CASE: EMPTY CHANNEL REMOVED
# ============================================================

def test_empty_channel_recreation():
    section("EDGE: EMPTY CHANNEL RECREATION")

    alice = IRCClient("alice")
    bob = IRCClient("bob")

    try:
        alice.connect()
        bob.connect()

        alice.register("alice")
        bob.register("bob")

        alice.send("JOIN #temporary")
        alice.read()

        alice.send("PART #temporary")
        alice.read()

        # Channel should now be deleted.
        # Bob joining it should create a fresh channel and become operator.
        bob.send("JOIN #temporary")

        response = bob.read()

        check(
            "Empty channel can be created again",
            "JOIN #temporary" in response
            or "JOIN :#temporary" in response,
            response
        )

        bob.clear()

        # If creator becomes operator this should succeed.
        bob.send("MODE #temporary +i")

        response = bob.read()

        check(
            "Creator of recreated channel is operator",
            " 482 " not in response,
            response
        )

    finally:
        alice.close()
        bob.close()


# ============================================================
# EDGE CASE: OPERATOR LEAVES
# ============================================================

def test_operator_leaves():
    section("EDGE: OPERATOR LEAVES")

    alice = IRCClient("alice")
    bob = IRCClient("bob")

    try:
        alice.connect()
        bob.connect()

        alice.register("alice")
        bob.register("bob")

        # Alice creates channel and should be operator.
        alice.send("JOIN #op-leave")
        alice.read()

        bob.send("JOIN #op-leave")
        bob.read()

        alice.clear()
        bob.clear()

        alice.send("PART #op-leave :bye")
        alice.read()
        bob.read()

        # Your Channel::removeMember() calls ensureOperator(),
        # so Bob should become an operator.
        bob.send("MODE #op-leave +i")

        response = bob.read()

        check(
            "Remaining member becomes operator",
            " 482 " not in response,
            response
        )

    finally:
        alice.close()
        bob.close()


# ============================================================
# EDGE CASE: SERVER HANDLES MANY CLIENTS
# ============================================================

def test_many_clients():
    section("EDGE: MANY CLIENTS")

    clients = []

    try:
        number_of_clients = 20

        for i in range(number_of_clients):
            client = IRCClient("client%d" % i)
            client.connect()

            response = client.register(
                "user%d" % i
            )

            check(
                "Register client %d" % i,
                " 001 user%d " % i in response,
                response
            )

            clients.append(client)

        check(
            "Server alive with %d clients" % number_of_clients,
            server_alive()
        )

        # All join one channel.
        for i, client in enumerate(clients):
            client.send("JOIN #stress")
            client.read()

        for client in clients:
            client.clear()

        clients[0].send(
            "PRIVMSG #stress :broadcast stress test"
        )

        time.sleep(0.3)

        received = 0

        for client in clients[1:]:
            response = client.read()

            if "broadcast stress test" in response:
                received += 1

        check(
            "Broadcast reaches all other clients",
            received == number_of_clients - 1,
            "Received: %d / %d" % (
                received,
                number_of_clients - 1
            )
        )

    finally:
        for client in clients:
            client.close()


# ============================================================
# EDGE CASE: CRLF SPLIT BETWEEN TCP PACKETS
# ============================================================

def test_split_crlf():
    section("EDGE: SPLIT CRLF")

    client = IRCClient("split-crlf")

    try:
        client.connect()
        client.clear()

        client.sock.sendall(
            ("PASS " + PASSWORD).encode()
        )

        client.sock.sendall(b"\r")

        time.sleep(0.05)

        client.sock.sendall(b"\nNICK splitcrlf\r")

        time.sleep(0.05)

        client.sock.sendall(
            b"\nUSER splitcrlf 0 * :Split CRLF\r"
        )

        time.sleep(0.05)

        client.sock.sendall(b"\n")

        time.sleep(0.2)

        response = client.read()

        check(
            "CRLF split between TCP packets",
            " 001 splitcrlf " in response,
            response
        )

    finally:
        client.close()

# ============================================================================
# HANDLE PING
# ============================================================================

def test_ping_pong():
    section("PING / PONG")

    client = IRCClient("ping")

    try:
        client.connect()
        client.register("pinguser")
        client.clear()

        client.send("PING :hello123")

        response = client.read()

        check(
            "PING receives PONG",
            "PONG" in response
            and "hello123" in response,
            response
        )

        client.send("PING")

        response = client.read()

        check(
            "PING without parameter -> 409",
            " 409 " in response,
            response
        )

        check(
            "Server alive after PING",
            server_alive()
        )

    finally:
        client.close()
        
def test_slow_client_flood():
    section("EDGE: SLOW CLIENT / FLOOD")

    slow = IRCClient("slow")
    sender = IRCClient("flooder")
    probe = None

    try:
        slow.connect()
        slow.register("slowclient")
        slow.clear()

        sender.connect()
        sender.register("flooder")
        sender.clear()

        channel = "#floodtest"

        slow.send("JOIN " + channel)
        time.sleep(0.2)
        slow.clear()

        sender.send("JOIN " + channel)
        time.sleep(0.2)

        slow.clear()
        sender.clear()

        # --------------------------------------------------
        # slow більше НЕ читає зі свого socket.
        #
        # Це імітує nc, який evaluator зупинив Ctrl+Z.
        # TCP connection залишається відкритим.
        # --------------------------------------------------

        payload = "X" * 400

        flood_count = 500

        print(
            "Flooding channel with %d messages while "
            "slow client is not reading..." % flood_count
        )

        for i in range(flood_count):
            message = (
                "PRIVMSG "
                + channel
                + " :flood_"
                + str(i)
                + "_"
                + payload
            )

            sender.send(message)

        # --------------------------------------------------
        # Поки slow client не читає, перевіряємо,
        # що сервер НЕ завис.
        # --------------------------------------------------

        time.sleep(0.5)

        check(
            "Server alive while client is not reading",
            server_alive()
        )

        # --------------------------------------------------
        # Новий клієнт повинен мати можливість
        # підключитися і зареєструватися.
        # --------------------------------------------------

        probe = IRCClient("probe")
        probe.connect()
        probe.register("probeclient")
        probe.clear()

        probe.send("JOIN #floodtest")
        time.sleep(0.2)

        response = probe.read()

        check(
         "New client works during flood",
         "JOIN" in response or "353" in response or "366" in response,
         response
        )

        # --------------------------------------------------
        # Тепер slow client знову починає читати.
        # Це аналог `fg` після Ctrl+Z.
        # --------------------------------------------------

        received = ""

        end_time = time.time() + 5.0

        while time.time() < end_time:
            try:
                chunk = slow.sock.recv(65536)

                if not chunk:
                    break

                received += chunk.decode(
                    "utf-8",
                    errors="ignore"
                )

                # Не обов'язково чекати всі 5000.
                # Достатньо довести, що queued data
                # почала приходити після resume.
                if "flood_" in received:
                    break

            except socket.timeout:
                continue
            except Exception:
                break

        check(
            "Stopped client receives queued messages after resume",
            "flood_" in received,
            "received %d bytes" % len(received)
        )

        # --------------------------------------------------
        # Після resume сервер все ще повинен працювати.
        # --------------------------------------------------

        check(
            "Server alive after slow client resumes",
            server_alive()
        )

    except Exception as e:
        check(
            "Slow client flood test",
            False,
            str(e)
        )

    finally:
        try:
            slow.close()
        except Exception:
            pass

        try:
            sender.close()
        except Exception:
            pass

        if probe is not None:
            try:
                probe.close()
            except Exception:
                pass

# ============================================================
# MAIN
# ============================================================

def main():
    global passed
    global failed

    print()
    print("============================================================")
    print("                 FT_IRC AUTOMATIC TESTER")
    print("============================================================")
    print("Executable : " + SERVER_EXEC)
    print("Port       : " + str(PORT))
    print("Password   : " + PASSWORD)
    print("============================================================")

    if not os.path.exists(SERVER_EXEC):
        print()
        print("ircserv not found.")
        print("Run: make")
        return 1

    if not start_server():
        return 1

    try:
        #test_registration()

        #check(
        #    "Server alive after registration",
        #    server_alive()
        #)

        #test_wrong_password()
        #test_duplicate_nick()

        #test_channel()
        #test_private_message()

        #test_part()
        #test_join_zero()

        #test_kick()
        #test_mode_key()
        #test_invite()

        #test_partial_command()
        #test_abrupt_disconnect()

        #test_quit()
        #test_ping_pong()

        #test_split_crlf()
        #test_many_clients()
        #test_operator_leaves()
        #test_empty_channel_recreation()
        #test_quit_broadcast()
        #test_nick_change()
        #test_message_after_part()
        #test_privmsg_unknown_channel()
        #test_privmsg_unknown_nick()
        #test_part_not_member()
        #test_part_nonexistent_channel()
        #test_duplicate_join()
        #test_case_insensitive_nick()
        #test_disconnect_mid_command()
        #test_incomplete_client_does_not_block()
        #test_multiple_commands_one_packet()
        #test_missing_parameters()
        #test_commands_before_registration()
        test_slow_client_flood()

    finally:
        stop_server()

    print()
    print("============================================================")
    print("                        RESULT")
    print("============================================================")
    print(
        "\033[92mPASSED: %d\033[0m" % passed
    )
    print(
        "\033[91mFAILED: %d\033[0m" % failed
    )
    print("============================================================")

    if failed == 0:
        print(
            "\033[92mALL TESTS PASSED\033[0m"
        )
        return 0

    return 1


if __name__ == "__main__":
    sys.exit(main())