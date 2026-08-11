#include "../incs/Channel.hpp"

Channel::Channel(const std::string& name) : _name(name) {
    _name = name;
    _topicRestriction = false;
    _inviteOnly = false;
    _userLimit = 0;
    _operator.clear();
    _key.clear();
    _topic.clear();
    _clients.clear();
    _invited.clear();
}

Channel::~Channel() {}

// Getters
const std::string& Channel::getKey() const                  { return _key; }
const std::string& Channel::getName() const                 { return _name; }
const std::string& Channel::getTopic() const                { return _topic; }
const std::map<int, Client*>& Channel::getClients() const   { return _clients; }
size_t Channel::getUserLimit() const                        { return _userLimit; }
std::string Channel::getModes() const {
    std::string modes = "+";
    if (_inviteOnly) modes += "i";
    if (_topicRestriction) modes += "t";
    if (hasUserLimit()) modes += "l";
    if (hasKey()) modes += "k";
    return modes;
}

// Client management
void Channel::addClient(Client* client) {
    _clients[client->getFd()] = client;
}
void Channel::addOperator(Client* client) {
    _operator.push_back(client);
}
void Channel::removeClient(Client* client) {
    std::map<int, Client*>::iterator it = _clients.find(client->getFd());
    if (it != _clients.end()) {
        _clients.erase(it);
    }
}
void Channel::removeOperator(Client* client) {
    _operator.erase(std::remove(_operator.begin(), _operator.end(), client), _operator.end());
}

// Status checks
bool Channel::hasClient(Client* client) const {
    return _clients.find(client->getFd()) != _clients.end();
}
bool Channel::hasKey() const {
    return !_key.empty();
}
bool Channel::hasUserLimit() const {
    return _userLimit > 0;
}
bool Channel::hasAnyOperator() const {
    return !_operator.empty();
}
bool Channel::isInviteOnly() const {
    return _inviteOnly;
}
bool Channel::isInvited(Client* client) const {
    return std::find(_invited.begin(), _invited.end(), client) != _invited.end();
}
bool Channel::isOperator(Client* client) const {
    return std::find(_operator.begin(), _operator.end(), client) != _operator.end();
}
bool Channel::isTopicRestricted() const {
    return _topicRestriction;
}

// Setters
void Channel::setKey(const std::string& key)        { _key = key; }
void Channel::setName(const std::string& name)      { _name = name; }
void Channel::setTopic(const std::string& topic)    { _topic = topic; }
void Channel::setUserLimit(size_t limit)            { _userLimit = limit; }
void Channel::setInviteOnly(bool status)            { _inviteOnly = status; }
void Channel::setTopicRestriction(bool restrict)    { _topicRestriction = restrict; }
 
// Mode management
void Channel::addInvite(Client* client) {
    _invited.push_back(client);
}
void Channel::removeInvite(Client* client) {
    _invited.erase(std::remove(_invited.begin(), _invited.end(), client), _invited.end());
}
void Channel::removeKey() {
    _key.clear();
}
void Channel::removeUserLimit() {
    _userLimit = 0;
}