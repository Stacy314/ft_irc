*This project has been created as part of the 42 curriculum by apechkov, oltolmac, ishchyro.

# ft_irc / ircserv

## Description

`ircserv` is a small Internet Relay Chat server written in C++98. It accepts multiple TCP clients, authenticates them with a server password, lets them set a nickname and username, join channels, exchange private and channel messages, and use the required channel-operator commands.

The server is intentionally single-process and event-driven. It uses non-blocking sockets and one `poll()` loop to handle listening, reading, and writing.

## Features

- TCP IPv4 server launched as `./ircserv <port> <password>`
- Non-blocking client sockets
- One `poll()`-based event loop
- Aggregates partial packets until full IRC lines are received
- Authentication with `PASS`
- Registration with `NICK` and `USER`
- Channels with automatic first-user operator status
- Private messages and channel messages
- Required operator commands:
  - `KICK`
  - `INVITE`
  - `TOPIC`
  - `MODE +i/-i`
  - `MODE +t/-t`
  - `MODE +k/-k`
  - `MODE +o/-o`
  - `MODE +l/-l`
- Basic compatibility helpers for common clients: `CAP`, `PING`, `WHO`, `NAMES`, MOTD numerics

## Instructions

Compile:

```sh
make
```

Run:

```sh
./ircserv 6667 secret
```

Connect with `nc`:

```sh
nc -C 127.0.0.1 6667
PASS secret
NICK alice
USER alice 0 * :Alice Example
JOIN #chat
PRIVMSG #chat :hello everyone
```

This server waits until a newline is received before processing the rebuilt command.

## Example client workflow

Client 1:

```irc
PASS secret
NICK alice
USER alice 0 * :Alice
JOIN #room
TOPIC #room :Welcome
MODE #room +i
```

Client 2:

```irc
PASS secret
NICK bob
USER bob 0 * :Bob
JOIN #room
```

If `#room` is invite-only, Alice can run:

```irc
INVITE bob #room
```

## Resources

- RFC 1459: Internet Relay Chat Protocol
- RFC 2812: IRC Client Protocol
- `man socket`
- `man bind`
- `man listen`
- `man accept`
- `man poll`
- `man fcntl`
- Beej's Guide to Network Programming

