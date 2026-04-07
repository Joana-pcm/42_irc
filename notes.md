## Guides and references

-> **IRC guide**:
	
	For learning the concepts:
		https://medium.com/@mohamedsarda/ft-irc-channels-and-command-management-ff1ff3758a0b

	This one is good to help structure the project:
		https://medium.com/@afatir.ahmedfatir/small-irc-server-ft-irc-42-network-7cee848de6f9

-> How to work with **sockets**:

	https://www.geeksforgeeks.org/cpp/socket-programming-in-cpp/
	https://www.geeksforgeeks.org/cpp/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/

## What we'll need:

-> [] **client class**, to store client information:

	- username, nickname, realname, hostname, password (all strings), clientfd(int)
	- method to know if they are authenticated
	- get and set methods

-> [] **server class**, to store server information:

	- port(int), serversocketfd(int), password(string), 

-> [] **channel class**, to store channel information:

	- channelfd
	- the command methods (KICK, JOIN, PRIVMSG, PART, QUIT, MODE, INVITE, TOPIC)

-> [] a way to store the automated replies, whether it's on a separate replies class, or as a private structure on the client class