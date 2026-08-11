# IRC

PLANNING

## Phase I: Core Engine

Get the server to run without crashing and ensure we are following the rule of using only one poll() with non-blocking file descriptors.

Partner A (the socket wrangler): writes the code to create the socket, bind it to the listening port, and set up the main poll() loop.

Partner B (the state keeper): designs the Client C++ class. This class needs to store the client’s socket file descriptor, their current state (e.g., connected, authenticated), and a string buffer to their incoming messages.

## Phase II: Communication & Parsing

Since network packets can arrive in pieces, aggregate data before processing it.

Partner A (buffer manager): writes the logic inside our poll() loop to read data using recv(). If a message doesn’t end with a \r\n (the standard IRC line ending), Partner’s A code must save it in the Client’s buffer until the rest of the message arrives.

Partner B (the translator): writes the parsing engine. Once Partner A’s code can tell there is a full line of text, Partner B’s code takes that string (e.g., “JOIN #42”) and splits it into a Command (JOIN) and an Argument (#42).

## Phase III: Chat Features

Once the server can read commands cleanly, assign different IRC commands to each person. Must implement specific features to pass the evaluation.

Partner A (User Setup & Messaging):

- Authentication: Implement the password check (PASS).
- Registration: Handle setting the nickname and username (NICK, USER).
- Chatting: Implement sending and receiving private messages (PRIVMSG).

Partner B (Rooms & Operators):

- Channel management: Build the Channel class and implement joining (JOIN).
- Basic moderation: Implement kicking users (KICK), inviting users (INVITE), and changing the topic (TOPIC).

Together:

The MODE command: most complex command in the mandatory part.  Implement several flags: I (invite-only), t (topic restrictions), k (channel password), o (operator privileges), and l (user limits).
