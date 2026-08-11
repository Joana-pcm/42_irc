#pragma once

#include <iostream>
#include <iomanip>
#include <vector>
#include <set>
#include <map>
#include <iterator>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <cstdlib>
#include <poll.h>
#include <fcntl.h>
#include <csignal>
#include <sstream>
#include <algorithm>
#include "Client.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "Num.hpp"

class Client;
class Channel;

class Server {
    public:
        Server(int port, std::string pass);
        ~Server();

        void    initServer();
        void    run();
        void    dispatchCommand(const Message& msg, Client* client);
        typedef void (Server::*CommandHandler)(Client&, const Message&); // to get function pointers

    private:
        int                                     _port;
        int                                     _serverSocket;
        std::string                             _serverName;
        std::string                             _password;
        std::map<int, Client*>                  _clients;
        std::vector<Channel*>                   _channels;
        std::vector<struct pollfd>              _pollFds;
        std::map<std::string, CommandHandler>   _commandHandlers;

        void        _acceptNewClient();
        void        _handleClientData(size_t index);
        void        _flushClientOutput(size_t index);

        // notice handling
        void        sendNum(Client* client, int errorNum, const std::string& message);
        void        sendWelcome(Client* client);
        void        sendNameReply(Client* client, Channel* channel);

        // validation
        bool        isValidNickname(const std::string& nickname);
        bool        isValidChannelName(const std::string& channelName);
        bool        isNicknameInUse(const std::string& nickname);

        // utility
        Channel*    findChannelByName(const std::string& name);
        Client*     findClientByNickname(const std::string& nickname);
        Client*     findClientByFd(int fd);
        void        broadcastToAllChannels(const std::string& message, Client* subject, Client* exclude = NULL);
        void        broadcastToChannel(const std::string& message, Channel* channel, Client* excludeClient = NULL);
        void        disconnectClient(size_t index);
        void        removeClientFromChannel(Client* client, Channel* channel);
        void        pollEventsUpdate();

        // command handling
        void        registerCommands();
        // --> handlers
        void        handleNick(Client& client, const Message& msg);
        void        handleUser(Client& client, const Message& msg);
        void        handlePass(Client& client, const Message& msg);
        void        handleJoin(Client& client, const Message& msg);
        void        handlePart(Client& client, const Message& msg);
        void        handlePrivmsg(Client& client, const Message& msg);
        void        handleTopic(Client& client, const Message& msg);
        void        handleInvite(Client& client, const Message& msg);
        void        handleKick(Client& client, const Message& msg);
        void        handleQuit(Client& client, const Message& msg);
        void        handleMode(Client& client, const Message& msg);
};

    // utility functions
    std::vector<std::string> split(const std::string& str, char delimiter);
    bool ircEquals(const std::string& a, const std::string& b);