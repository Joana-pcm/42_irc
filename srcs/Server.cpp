#include "../incs/Server.hpp"

static bool running = true;
static void sigHandler(int signum) {
    (void)signum;
    running = false;
}  

Server::Server(int port, std::string pass) : _port(port), _serverSocket(-1), _password(pass){
    _serverName = "IRCServer";
    registerCommands();
}
// -1 "not created yet"

Server::~Server() {
    for (size_t i = 0; i < _pollFds.size(); ++i)
        close(_pollFds[i].fd);
    for (size_t i = 0; i < _channels.size(); ++i)
        delete _channels[i];
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
        delete it->second;
}

// _pollFds holds socket and all client sockets

void Server::initServer() {
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket < 0) {
        std::cerr << "Error: socket() failed." << std::endl;
        std::exit(1);
    }
    // create endpoint

    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error: setsockopt() failed." << std::endl;
        close(_serverSocket);
        std::exit(1);
    }
    // server reuse
    
    if (fcntl(_serverSocket, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << "Error: fcntl() failed." << std::endl;
        close(_serverSocket);
        std::exit(1);
    }
    // non-blocking helper

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family         = AF_INET;
    addr.sin_addr.s_addr    = INADDR_ANY;
    addr.sin_port           = htons(_port);

    if (bind(_serverSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "Error: bind() failed." << std::endl;
        close(_serverSocket);
        std::exit(1);
    }
    // assign port traffic to socket

    if (listen(_serverSocket, SOMAXCONN) < 0) {
        std::cerr << "Error: listen() failed." << std::endl;
        close(_serverSocket);
        std::exit(1);
    }
    // SOMAXCONN: max backlog pending connections the OS allows
    // at this moment, passive socket: doesn't send or receive data, only accepts new connections

    struct pollfd pfd;
    pfd.fd      = _serverSocket;
    pfd.events  = POLLIN;
    pfd.revents = 0;
    _pollFds.push_back(pfd);
    // POLLIN means server awakes from passive mode when someone wants to connect, otherwise it will get back to "sleep"
    // revents + POLLIN true means there is data waiting to be read on that fd
    // push_back, method from std::vector that appends a new element to the end of the vector

    std::cout << "Server started on port " << _port << std::endl;
}

void Server::run() {
    signal(SIGINT, sigHandler);
    signal(SIGQUIT, sigHandler);

    while (running) {
        int ready = poll(&_pollFds[0], _pollFds.size(), -1);
        if (ready < 0) {
            if(!running)
                break ;
            std::cerr << "Error: poll() failed." << std::endl;
            break ;
        }
        for (size_t i = _pollFds.size(); i > 0; --i) {
            size_t index = i - 1;
            int fd = _pollFds[index].fd;

            if (_pollFds[index].revents & POLLIN) {
                if (fd == _serverSocket)
                    _acceptNewClient();
                else
                {
                    _handleClientData(index);
                    if (index < _pollFds.size() && _pollFds[index].fd == fd) // check if client still exists after handling data
                    {
                        Client* client = findClientByFd(fd);
                        if (client && client->isPendingQuit()) {
                            disconnectClient(index);
                        }
                    }
                }
            }
            if (index < _pollFds.size() && _pollFds[index].fd == fd
            && fd != _serverSocket && (_pollFds[index].revents & POLLOUT))
                _flushClientOutput(index);
        }
    }
    std::cout << "\nServer shutting down." << std::endl;
}

void Server::_acceptNewClient() {
    struct sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);

    int clientFd = accept(_serverSocket, (struct sockaddr *)&clientAddr, &len);
    if (clientFd < 0)
        return ;
    
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
        std::cerr << "Error: fcntl() on client failed." << std::endl;
        close(clientFd);
        return;
    }
    // making client's non-blocking

    struct pollfd   pfd;
    pfd.fd          = clientFd;
    pfd.events      = POLLIN;
    pfd.revents     = 0;
    _pollFds.push_back(pfd);

    // add new client to the vector (construct in-place to keep ownership)
    Client *newClient = new Client(clientFd);
    newClient->setConnected(true);
    newClient->setHostname(inet_ntoa(clientAddr.sin_addr));
    _clients[clientFd] = newClient;

    // add client to poll() watchlist
    std::cout << "Client connected [fd=" << clientFd << "]" << std::endl;
}

// changed _handleClientData to work with _clients vector,
// so we can manage client-specific data and states in the future
// (like authentication, nicknames, etc.)

void Server::_handleClientData(size_t index) {
    int fd = _pollFds[index].fd;
    Client* client = findClientByFd(fd);

    if (!client)
        return ;

    char buffer[BUFFER_SIZE];
    std::memset(buffer, 0, sizeof(buffer));
    ssize_t bytes = recv(fd, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytes <= 0) {
        disconnectClient(index);
        return ;
    }

    buffer[bytes] = '\0';
    client->appendToInBuffer(buffer, bytes);
    std::vector<std::string> lines = client->extractCompleteLines();
    for (size_t i = 0; i < lines.size(); ++i) {
        Message msg = parseLine(lines[i]);
        if (!msg.command.empty())
            dispatchCommand(msg, client);
    }
    pollEventsUpdate();
}

void Server::disconnectClient(size_t index) {
    int fd = _pollFds[index].fd;
    Client* client = findClientByFd(fd);
    if (!client)
        return ;
    
    for (size_t i = _channels.size(); i > 0; --i)
    {
        if (_channels[i - 1]->hasClient(client))
            removeClientFromChannel(client, _channels[i - 1]);
    }
    std::cout << "Client disconnected [fd=" << client->getFd() << "]" << std::endl;
    close(fd);
    delete client;
    _pollFds.erase(_pollFds.begin() + index);
    _clients.erase(fd);
}

void Server::_flushClientOutput(size_t index) {
    int fd = _pollFds[index].fd;
    Client* client = findClientByFd(fd);
    if (!client)
        return ;
    const std::string& out = client->getOutBuffer();
    if (out.empty())
    {
        _pollFds[index].events &= ~POLLOUT; // clear POLLOUT flag
        return ;
    }
    ssize_t bytesSent = send(fd, out.c_str(), out.size(), 0);
    if (bytesSent > 0) {
        client->clearOutBuffer(bytesSent);
    }
    if (client->getOutBuffer().empty()) {
        _pollFds[index].events &= ~POLLOUT;
    }
}

void Server::pollEventsUpdate() {
    for (size_t i = 0; i < _pollFds.size(); ++i) {
        int fd = _pollFds[i].fd;
        Client* client = findClientByFd(fd);
        if (client && !client->getOutBuffer().empty()) {
            _pollFds[i].events |= POLLOUT; // set POLLOUT flag
        }
    }
}

void Server::dispatchCommand(const Message& msg, Client* client) {
    if (!client->isAuthenticated()
        && (msg.command != "NICK" && msg.command != "USER" && msg.command != "PASS")) {
        sendNum(client, ERR_NOTREGISTERED, "You have not registered");
        return ;
    }

    std::map<std::string, CommandHandler>::iterator it = _commandHandlers.find(msg.command);
    if (it == _commandHandlers.end()) {
        sendNum(client, ERR_UNKNOWNCOMMAND, "Unknown command");
        return ;
    }

    bool wasAuthenticated = client->isAuthenticated();
    CommandHandler handler = it->second;
    (this->*handler)(*client, msg);

    if (!wasAuthenticated && client->isAuthenticated())
        sendWelcome(client);
}


void Server::sendNum(Client* client, int errorNum, const std::string& message) {
    std::ostringstream oss;
    oss << ":" << _serverName << " " << std::setfill('0') << std::setw(3) 
    << errorNum << " " 
    << (client->getNickname().empty() ? "*" : client->getNickname()) 
    << " " << message << "\r\n"; 
    client->queueOutgoing(oss.str());
}

void Server::sendWelcome(Client* client) {
    sendNum(client, RPL_WELCOME, ":Welcome to the IRC server, " + client->getNickname()
            + "!" + client->getUsername() + "@" + client->getHostname());
            sendNum(client, RPL_YOURHOST, ":Your host is " + _serverName + ", running version 1.0");
            sendNum(client, RPL_CREATED, ":This server was created on " + std::string(__DATE__) + " at " + std::string(__TIME__));
            sendNum(client, RPL_MYINFO, ":Server info: IRCServer 1.0");
        }
        
        void Server::sendNameReply(Client* client, Channel* channel) {
            const std::map<int, Client*>& clientsInChannel = channel->getClients();
            std::string nameList;
            for (std::map<int, Client*>::const_iterator it = clientsInChannel.begin(); it != clientsInChannel.end(); ++it) {
                Client* currentClient = it->second;
                if (channel->isOperator(currentClient))
                nameList += "@";
        nameList += currentClient->getNickname();
        if (it != clientsInChannel.end())
        nameList += " ";
    }
    sendNum(client, RPL_NAMREPLY, "= " + channel->getName() + " :" + nameList);
    sendNum(client, RPL_ENDOFNAMES, channel->getName() + " :End of /NAMES list");
}

void Server::broadcastToAllChannels(const std::string& message, Client* subject, Client* exclude) {
    std::set<Client*> notified;
    for (size_t i = 0; i < _channels.size(); ++i) {
        Channel* channel = _channels[i];
        if (!channel->hasClient(subject)){
            continue ;
        }
        const std::map<int, Client*>& clientsInChannel = channel->getClients();
        for (std::map<int, Client*>::const_iterator it = clientsInChannel.begin(); it != clientsInChannel.end(); ++it) {
            Client* client = it->second;
            if (client != exclude && notified.find(client) == notified.end()) {
                client->queueOutgoing(message);
                notified.insert(client);
            }
        }
    }
}

void Server::broadcastToChannel(const std::string& message, Channel* channel, Client* excludeClient) {
    const std::map<int, Client*>& clientsInChannel = channel->getClients();
    for (std::map<int, Client*>::const_iterator it = clientsInChannel.begin(); it != clientsInChannel.end(); ++it) {
        Client* client = it->second;
        if (client != excludeClient) {
            client->queueOutgoing(message);
        }
    }
}
