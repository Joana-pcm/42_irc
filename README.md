*This project has been created as part of the 42 curriculum by <login1>.*

# ft_irc

## Description

**ft_irc** is a custom implementation of an IRC (Internet Relay Chat) server, written in C++98 from scratch — no external IRC libraries — as part of the 42 core curriculum.

The goal of the project is to build a working, RFC-inspired IRC server capable of handling multiple clients at once using a **single-threaded, non-blocking event loop** (`poll()`), rather than spawning a thread or process per connection. The server must be robust enough to interoperate with a real, unmodified IRC client (HexChat, irssi, etc.), correctly implementing connection registration, channel management, operator privileges, and the core set of IRC commands.

In short, the project is about understanding:
- How TCP servers handle many concurrent clients without blocking, using `poll()`/`select()`-style I/O multiplexing.
- The IRC protocol itself: message framing, command syntax, numeric replies, and channel semantics.
- Clean C++ design for a stateful, event-driven system (clients, channels, and command dispatch).

### Features

- Non-blocking TCP server driven by a single `poll()` loop covering the listening socket and every connected client.
- Per-client buffering that correctly reassembles IRC messages split across multiple TCP reads.
- Connection registration: `PASS`, `NICK`, `USER`, with full RFC-style welcome burst on success.
- Channel management: `JOIN`, `PART`, `TOPIC`, `KICK`, `INVITE`.
- Channel modes: `i` (invite-only), `t` (topic restricted to operators), `k` (key), `o` (operator), `l` (user limit).
- Messaging: `PRIVMSG` to channels and to individual users by nickname.
- Graceful handling of client disconnects (clean `QUIT` and abrupt drops), including automatic operator reassignment when the last operator leaves a channel.

## Instructions

### Compilation

```bash
make
```

This builds the `ircserv` binary using C++98, with the project's Makefile (`all`, `clean`, `fclean`, `re` rules).

### Execution

```bash
./ircserv <port> <password>
```

- `<port>` — the TCP port the server listens on.
- `<password>` — the connection password clients must provide via `PASS` before completing registration.

Example:

```bash
./ircserv 6667 mypassword
```

### Connecting

Any standard IRC client works. For example, with **HexChat**:

1. Open the Network List and add a new network.
2. Set the server address to `127.0.0.1/<port>`.
3. Set the network password to `<password>`.
4. Disable SSL (the server does not implement TLS).
5. Connect, then set a nickname and username as prompted.

For quick manual testing without a GUI client, `netcat` can be used to send raw IRC lines:

```bash
nc -C 127.0.0.1 <port>
```

```
PASS mypassword
NICK yournick
USER yourusername 0 * :Your Real Name
JOIN #channel
```

## Resources

Classic references used to understand IRC and socket programming concepts:

- [RFC 1459 — Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459)
- [RFC 2812 — Internet Relay Chat: Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812)
- [ft_irc: Channels and Command Management](https://medium.com/@mohamedsarda/ft-irc-channels-and-command-management-ff1ff3758a0b)
- [Small IRC Server (ft_irc, 42 Network)](https://medium.com/@afatir.ahmedfatir/small-irc-server-ft-irc-42-network-7cee848de6f9)
- [Socket Programming in C++ (GeeksforGeeks)](https://www.geeksforgeeks.org/cpp/socket-programming-in-cpp/)
- [Socket Programming in C/C++: Handling Multiple Clients on a Server Without Multi-Threading (GeeksforGeeks)](https://www.geeksforgeeks.org/cpp/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/)

### AI usage

AI (Claude, Anthropic) was used throughout this project as a design and debugging aid, not as a substitute for writing or understanding the code. Specifically, it was used for:

- **Architecture guidance**: discussing how to structure `Server`, `Client`, `Channel`, and `Message` into separate responsibilities, and how to design the command-dispatch system (a `std::map` of command names to member-function-pointer handlers).
- **IRC protocol clarification**: understanding the exact message grammar (prefix/command/params/trailing parameter), the semantics of commands like `JOIN`, `PART`, `MODE`, and `KICK`, and which numeric replies apply to which situations.
- **Code review**: reviewing hand-written implementations of the `poll()` loop, client buffering, and command handlers to catch bugs — including an index-offset bug between two parallel vectors, a use-after-free when a client disconnected while still referenced by a channel's member list, uninitialized `Channel` members flagged by Valgrind, and missing `POLLOUT` handling that caused replies to be silently queued but never sent.
- **Debugging support**: interpreting Valgrind output and server logs to pinpoint the root cause of specific bugs (e.g. a stale pointer left in a channel after a client's `Client` object was deleted).

All resulting code was written, tested, and understood by the author; AI-suggested fixes were evaluated and adapted rather than copied blindly.*This project has been created as part of the 42 curriculum by <login1>.*

# ft_irc

## Description

**ft_irc** is a custom implementation of an IRC (Internet Relay Chat) server, written in C++98 from scratch — no external IRC libraries — as part of the 42 core curriculum.

The goal of the project is to build a working, RFC-inspired IRC server capable of handling multiple clients at once using a **single-threaded, non-blocking event loop** (`poll()`), rather than spawning a thread or process per connection. The server must be robust enough to interoperate with a real, unmodified IRC client (HexChat, irssi, etc.), correctly implementing connection registration, channel management, operator privileges, and the core set of IRC commands.

In short, the project is about understanding:
- How TCP servers handle many concurrent clients without blocking, using `poll()`/`select()`-style I/O multiplexing.
- The IRC protocol itself: message framing, command syntax, numeric replies, and channel semantics.
- Clean C++ design for a stateful, event-driven system (clients, channels, and command dispatch).

### Features

- Non-blocking TCP server driven by a single `poll()` loop covering the listening socket and every connected client.
- Per-client buffering that correctly reassembles IRC messages split across multiple TCP reads.
- Connection registration: `PASS`, `NICK`, `USER`, with full RFC-style welcome burst on success.
- Channel management: `JOIN`, `PART`, `TOPIC`, `KICK`, `INVITE`.
- Channel modes: `i` (invite-only), `t` (topic restricted to operators), `k` (key), `o` (operator), `l` (user limit).
- Messaging: `PRIVMSG` to channels and to individual users by nickname.
- Graceful handling of client disconnects (clean `QUIT` and abrupt drops), including automatic operator reassignment when the last operator leaves a channel.

## Instructions

### Compilation

```bash
make
```

This builds the `ircserv` binary using C++98, with the project's Makefile (`all`, `clean`, `fclean`, `re` rules).

### Execution

```bash
./ircserv <port> <password>
```

- `<port>` — the TCP port the server listens on.
- `<password>` — the connection password clients must provide via `PASS` before completing registration.

Example:

```bash
./ircserv 6667 mypassword
```

### Connecting

Any standard IRC client works. For example, with **HexChat**:

1. Open the Network List and add a new network.
2. Set the server address to `127.0.0.1/<port>`.
3. Set the network password to `<password>`.
4. Disable SSL (the server does not implement TLS).
5. Connect, then set a nickname and username as prompted.

For quick manual testing without a GUI client, `netcat` can be used to send raw IRC lines:

```bash
nc -C 127.0.0.1 <port>
```

```
PASS mypassword
NICK yournick
USER yourusername 0 * :Your Real Name
JOIN #channel
```

## Resources

Classic references used to understand IRC and socket programming concepts:

- [RFC 1459 — Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459)
- [RFC 2812 — Internet Relay Chat: Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812)
- [ft_irc: Channels and Command Management](https://medium.com/@mohamedsarda/ft-irc-channels-and-command-management-ff1ff3758a0b)
- [Small IRC Server (ft_irc, 42 Network)](https://medium.com/@afatir.ahmedfatir/small-irc-server-ft-irc-42-network-7cee848de6f9)
- [Socket Programming in C++ (GeeksforGeeks)](https://www.geeksforgeeks.org/cpp/socket-programming-in-cpp/)
- [Socket Programming in C/C++: Handling Multiple Clients on a Server Without Multi-Threading (GeeksforGeeks)](https://www.geeksforgeeks.org/cpp/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/)

### AI usage

AI (Claude, Anthropic) was used throughout this project as a design and debugging aid, not as a substitute for writing or understanding the code. Specifically, it was used for:

- **IRC protocol clarification**: understanding the exact message grammar (prefix/command/params/trailing parameter), the semantics of commands like `JOIN`, `PART`, `MODE`, and `KICK`, and which numeric replies apply to which situations.
- **Debugging support**: interpreting Valgrind output and server logs to pinpoint the root cause of specific bugs (e.g. a stale pointer left in a channel after a client's `Client` object was deleted).

All resulting code was written, tested, and understood by the author; AI-suggested fixes were evaluated and adapted rather than copied blindly.