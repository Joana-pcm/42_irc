#pragma once

#include "Server.hpp"

class Client;

class Channel {
private:
    bool                    _topicRestriction;
    bool                    _inviteOnly;
    size_t                  _userLimit;
    std::string             _name;
    std::string             _key;
    std::string             _topic;
    std::vector<Client*>    _operator;
    std::vector<Client*>    _invited;
    std::map<int, Client*>  _clients;

public:
    Channel(const std::string& name);
    ~Channel();

    // manage clients
    void addClient(Client* client);
    void addOperator(Client* client);
    void removeClient(Client* client);
    void removeOperator(Client* client);

    // manage modes
    void addInvite(Client* client);
    void removeInvite(Client* client);
    void removeKey();
    void removeUserLimit();

    // check status
    bool hasClient(Client* client) const;
    bool hasKey() const;
    bool hasUserLimit() const;
    bool hasAnyOperator() const;
    bool isOperator(Client* client) const;
    bool isInviteOnly() const;
    bool isInvited(Client* client) const;
    bool isTopicRestricted() const;

    // getters
    size_t                          getUserLimit() const;
    std::string                     getModes() const;
    const std::string&              getName() const;
    const std::string&              getKey() const;
    const std::string&              getTopic() const;
    const std::map<int, Client*>&   getClients() const;

    // setters
    void setInviteOnly(bool status);
    void setKey(const std::string& key);
    void setUserLimit(size_t limit);
    void setName(const std::string& name);
    void setTopic(const std::string& topic);
    void setTopicRestriction(bool restrict);
};