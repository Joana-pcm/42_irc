#include "../incs/Server.hpp"

Channel* Server::findChannelByName(const std::string& name) {
    for (size_t i = 0; i < _channels.size(); ++i) {
        if (ircEquals(_channels[i]->getName(), name)) {
            return _channels[i];
        }
    }
    return NULL;
}

Client* Server::findClientByNickname(const std::string& nickname) {
    std::map<int, Client*>::iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it) {
        if (ircEquals(it->second->getNickname(), nickname)) {
            return it->second;
        }
    }
    return NULL;
}

// Validation functions

bool Server::isValidChannelName(const std::string& channelName) {
    if (channelName.empty() || channelName[0] != '#')
    return false;
    for (size_t i = 1; i < channelName.length(); ++i) {
        char c = channelName[i];
        if (!std::isalnum(c) && c != '-' && c != '_')
        return false;
    }
    return true;
}

bool Server::isValidNickname(const std::string& nickname) {
    // Check if the nickname is valid according to IRC rules
    if (nickname.empty() || nickname.length() > 9)
    return false;
    for (size_t i = 0; i < nickname.length(); ++i) {
        char c = nickname[i];
        if (!std::isalnum(c) && c != '-' && c != '_')
        return false;
    }
    return true;
}

bool Server::isNicknameInUse(const std::string& nickname) {
    std::map<int, Client*>::iterator it;
    for (it = _clients.begin(); it != _clients.end(); ++it) {
        if (ircEquals(it->second->getNickname(), nickname)) {
            return true;
        }
    }
    return false;
}

// Utility functions

Client* Server::findClientByFd(int fd) {
    std::map<int, Client*>::iterator it = _clients.find(fd);
    return (it != _clients.end()) ? it->second : NULL;
}

void Server::removeClientFromChannel(Client* client, Channel* channel) {
    if (channel->hasClient(client)) {
        bool wasOperator = channel->isOperator(client);
        channel->removeClient(client);
        if (wasOperator)
            channel->removeOperator(client);
        if (channel->getClients().empty()) {
            // If the channel is empty after removal, delete it
            std::vector<Channel*>::iterator it = std::find(_channels.begin(), _channels.end(), channel);
            if (it != _channels.end()) {
                delete *it;
                _channels.erase(it);
            }
            return ;
        }
        if (wasOperator && !channel->hasAnyOperator()) {
            // If the removed client was the operator, assign a new operator
            Client* newOperator = channel->getClients().begin()->second;
            channel->addOperator(newOperator);
            sendNum(newOperator, RPL_YOUREOPER, ":You are now a channel operator");
            std::string opMsg = ":" + _serverName + " MODE " + channel->getName() + " +o " + newOperator->getNickname() + " is now the channel operator\r\n";
            broadcastToChannel(opMsg, channel, NULL);
        }
        else
        {
            std::cout << "[DEBUG] Removed operator: " << channel->hasAnyOperator() << std::endl;
        }
    }
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

bool ircEquals(const std::string& a, const std::string& b) {
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    }
    return true;
}