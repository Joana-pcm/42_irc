#include "../incs/Server.hpp"

// Command handlers
// all commands are dispatched through this function, 
// which looks up the command in the map and calls the appropriate handler
// all other commands are ignored
void Server::registerCommands() {
    _commandHandlers["NICK"] = &Server::handleNick;
    _commandHandlers["USER"] = &Server::handleUser;
    _commandHandlers["PASS"] = &Server::handlePass;
    _commandHandlers["JOIN"] = &Server::handleJoin;
    _commandHandlers["PART"] = &Server::handlePart;
    _commandHandlers["PRIVMSG"] = &Server::handlePrivmsg;
    _commandHandlers["TOPIC"] = &Server::handleTopic;
    _commandHandlers["INVITE"] = &Server::handleInvite;
    _commandHandlers["KICK"] = &Server::handleKick;
    _commandHandlers["QUIT"] = &Server::handleQuit;
    _commandHandlers["MODE"] = &Server::handleMode;
}

// handle Nick
// parses and valdates the NICK command,
// creates or update the client's nickname as needed
void Server::handleNick(Client& client, const Message& msg) {
    if (msg.params.empty() || msg.params[0].empty()) {
        sendNum(&client, ERR_NONICKNAMEGIVEN, "No nickname given");
        return ;
    }

    const std::string& newNick = msg.params[0];

    if (!isValidNickname(newNick)) {
        sendNum(&client, ERR_INVALIDNICKNAME, "Invalid nickname");
        return ;
    }
    if (isNicknameInUse(newNick)) {
        sendNum(&client, ERR_NICKNAMEINUSE, "Nickname is already in use");
        return ;
    }

    std::string oldNick = client.getNickname();
    client.setNickname(newNick);
    
    if (client.isAuthenticated()) {
        // Notify other clients about the nickname change
        // only if the client is already authenticated
        std::string notification = ":" + oldNick + " NICK :" + newNick + "\r\n";
        broadcastToAllChannels(notification, &client);
    }
}

// handle User 
// parses and validates the USER command,
// sets the client's username and realname
// this is only allowed once, and only before the client is authenticated
void Server::handleUser(Client& client, const Message& msg) {
    if (client.isAuthenticated()) {
        sendNum(&client, ERR_ALREADYREGISTRED, "You are already registered");
        return ;
    }
    if (msg.params.size() < 4 || msg.params[0].empty() || msg.params[1].empty() 
    || msg.params[2].empty() || msg.params[3].empty()) {
        sendNum(&client, ERR_NEEDMOREPARAMS, "USER :Not enough parameters");
        return ;
    }
    client.setUsername(msg.params[2]);
    client.setRealname(msg.params[3]);
}

// handle Pass
// parses and validates the PASS command,
// sets the client's password if it matches the server's password
void Server::handlePass(Client& client, const Message& msg) {
    if (client.isAuthenticated()) {
        sendNum(&client, ERR_ALREADYREGISTRED, "You are already registered");
        return ;
    }
    if (msg.params.empty() || msg.params[0].empty()) {
        sendNum(&client, ERR_NEEDMOREPARAMS, "PASS :Not enough parameters");
        return ;
    }
    if (msg.params[0] != _password) {
        sendNum(&client, ERR_PASSWDMISMATCH, "Password incorrect");
        return ;
    }
    client.setHasPass(true);
}

// handle Join
// parses and validates the JOIN command,
// adds the client to the specified channel(s),
// checks for channel existence, key, invite-only status, and user limit,
// creating the channel if it doesn't exist, and broadcasting the join message
void Server::handleJoin(Client& client, const Message& msg) {
    if (msg.params.empty() || msg.params[0].empty()) {
        sendNum(&client, ERR_NEEDMOREPARAMS, "JOIN :Not enough parameters");
        return ;
    }

    std::vector<std::string> channelNames = split(msg.params[0], ',');
    std::vector<std::string> keys;

    if (msg.params.size() > 1) {
        keys = split(msg.params[1], ',');
    }

    for (size_t i = 0; i < channelNames.size(); ++i) {
        std::string& channelName = channelNames[i];
        std::string key = (i < keys.size()) ? keys[i] : "";
        bool isNewChannel = false;

        if (!isValidChannelName(channelName)) {
            sendNum(&client, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
            continue ;
        }
        Channel* channel = findChannelByName(channelName);
        if (!channel) {
            // Create new channel
            isNewChannel = true;
            channel = new Channel(channelName);
            _channels.push_back(channel);
        }
        else if (channel->hasClient(&client)) {
            sendNum(&client, ERR_USERONCHANNEL, channelName + " :You are already on that channel");
            continue ;
        }
        if (channel->isInviteOnly() && !channel->isInvited(&client)) {
            sendNum(&client, ERR_INVITEONLYCHAN, channelName + " :Cannot join channel (+i)");
            continue ;
        }
        if (channel->hasUserLimit() && channel->getClients().size() >= channel->getUserLimit()) {
            sendNum(&client, ERR_CHANNELISFULL, channelName + " :Cannot join channel (+l)");
            continue ;
        }
        if (channel->hasKey() && channel->getKey() != key) {
            sendNum(&client, ERR_BADCHANNELKEY, channelName + " :Cannot join channel (+k)");
            continue ;
        }
        // Add client to the channel
        channel->addClient(&client);
        if (channel->isInvited(&client)) 
            channel->removeInvite(&client);
        if (isNewChannel)
            channel->addOperator(&client);
        std::string joinMsg = ":" + client.getNickname() + " JOIN " + channelName + "\r\n";
        broadcastToChannel(joinMsg, channel, NULL);

        if (!channel->getTopic().empty()) {
            sendNum(&client, RPL_TOPIC, channelName + " :" + channel->getTopic());
        }
        else {
            sendNum(&client, RPL_NOTOPIC, channelName + " :No topic is set");
        }
        sendNameReply(&client, channel);
    }
}

void Server::handlePart(Client& client, const Message& msg) {
    if (msg.params.empty() || msg.params[0].empty()) {
        sendNum(&client, ERR_NEEDMOREPARAMS, "PART :Not enough parameters");
        return ;
    }

    std::vector<std::string> channelNames = split(msg.params[0], ',');
    std::string reason = (msg.params.size() > 1) ? msg.params[1] : client.getNickname();
    for (size_t i = 0; i < channelNames.size(); ++i) {
        std::string& channelName = channelNames[i];
        Channel* channel = findChannelByName(channelName);
        if (!channel) {
            sendNum(&client, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
            continue;
        }
        if (!channel->hasClient(&client)) {
            sendNum(&client, ERR_NOTONCHANNEL, channelName + " :You're not on that channel");
            continue;
        }
        std::string partMsg = ":" + client.getNickname() + "!" + client.getUsername() 
                                + "@" + client.getHostname() + " PART " + channelName + " :" + reason + "\r\n";
        broadcastToChannel(partMsg, channel, NULL);
        removeClientFromChannel(&client, channel);
    }
}

void Server::handlePrivmsg(Client& client, const Message& msg) {
    if (msg.params.size() < 2) {
        sendNum(&client, ERR_NEEDMOREPARAMS, "PRIVMSG :Not enough parameters");
        return ;
    }
    const std::string& target = msg.params[0];
    const std::string& message = msg.params[1];

    if (target.empty() || message.empty()) {
        sendNum(&client, ERR_NEEDMOREPARAMS, "PRIVMSG :Not enough parameters");
        return ;
    }

    Channel* channel = findChannelByName(target);
    if (channel) {
        if (!channel->hasClient(&client)) {
            sendNum(&client, ERR_CANNOTSENDTOCHAN, target + " :Cannot send to channel");
            return ;
        }
        std::string privMsg = ":" + client.getNickname() + " PRIVMSG " + target + " :" + message + "\r\n";
        broadcastToChannel(privMsg, channel, &client);
    } else {
        Client* targetClient = findClientByNickname(target);
        if (!targetClient) {
            sendNum(&client, ERR_NOSUCHNICK, target + " :No such nick/channel");
            return ;
        }
        std::string privMsg = ":" + client.getNickname() + " PRIVMSG "
                            + targetClient->getNickname() + " :" + message + "\r\n";
        targetClient->queueOutgoing(privMsg);
    }
}

void Server::handleTopic(Client& client, const Message& msg) {
    if (msg.params.empty() || msg.params[0].empty()) {
        sendNum(&client, ERR_NEEDMOREPARAMS, "TOPIC :Not enough parameters");
        return ;
    }
    Channel* channel = findChannelByName(msg.params[0]);
    if (!channel) {
        sendNum(&client, ERR_NOSUCHCHANNEL, msg.params[0] + " :No such channel");
        return ;
    }
    if (!channel->hasClient(&client)) {
        sendNum(&client, ERR_NOTONCHANNEL, msg.params[0] + " :You're not on that channel");
        return ;
    }
    if (msg.params.size() == 1) {
        // Requesting the topic
        std::string topic = channel->getTopic();
        if (topic.empty()) {
            sendNum(&client, RPL_NOTOPIC, msg.params[0] + " :No topic is set");
        } else {
            sendNum(&client, RPL_TOPIC, msg.params[0] + " :" + topic);
        }
    } else {
        // Setting the topic
        if (channel->isTopicRestricted() && !channel->isOperator(&client)) {
            sendNum(&client, ERR_CHANOPRIVSNEEDED, msg.params[0] + " :You're not channel operator");
            return ;
        }
        std::string newTopic = msg.params[1];
        channel->setTopic(newTopic);
        broadcastToChannel(":" + client.getNickname() + " TOPIC " + msg.params[0] + " :"
                            + newTopic + "\r\n", channel, NULL);
    }
}

void Server::handleInvite(Client& client, const Message& msg) {
    if (msg.params.size() < 2) {
        sendNum(&client, ERR_NEEDMOREPARAMS, "INVITE :Not enough parameters");
        return;
    }
    Channel* channel = findChannelByName(msg.params[1]);
    if (!channel) {
        sendNum(&client, ERR_NOSUCHCHANNEL, msg.params[1] + " :No such channel");
        return;
    }
	if (!channel->hasClient(&client)) {
		sendNum(&client, ERR_NOTONCHANNEL, msg.params[1] + " :You're not on that channel");
		return;
	}
    if (!channel->isOperator(&client)) {
        sendNum(&client, ERR_CHANOPRIVSNEEDED, msg.params[1] + " :You're not channel operator");
        return;
    }
    Client* target = findClientByNickname(msg.params[0]);
    if (!target) {
        sendNum(&client, ERR_NOSUCHNICK, msg.params[0] + " :No such nick/channel");
        return;
    }
    if (channel->hasClient(target)) {
        sendNum(&client, ERR_USERONCHANNEL, msg.params[0] + " " + msg.params[1] + " :They are already on that channel");
        return;
    }
    channel->addInvite(target);
    sendNum(&client, RPL_INVITING, msg.params[0] + " :" + msg.params[1]);
    std::string notice = ":" + client.getNickname() + " INVITE " + target->getNickname() + " :" + channel->getName() + "\r\n";
    target->queueOutgoing(notice);
}

void Server::handleKick(Client& client, const Message& msg) {
    if (msg.params.size() < 2) {
        sendNum(&client, ERR_NEEDMOREPARAMS, "KICK :Not enough parameters");
        return;
    }
    Channel* channel = findChannelByName(msg.params[0]);
    if (!channel) {
        sendNum(&client, ERR_NOSUCHCHANNEL, msg.params[0] + " :No such channel");
        return;
    }
	if (!channel->hasClient(&client)) {
		sendNum(&client, ERR_NOTONCHANNEL, msg.params[0] + " :You're not on that channel");
		return;
	}
    if (!channel->isOperator(&client)) {
        sendNum(&client, ERR_CHANOPRIVSNEEDED, msg.params[0] + " :You're not channel operator");
        return;
    }
    Client* target = findClientByNickname(msg.params[1]);
    if (!target || !channel->hasClient(target)) {
        sendNum(&client, ERR_USERNOTINCHANNEL, msg.params[1] + " " + msg.params[0] + " :They aren't on that channel");
        return ;
    }
    if (target == &client) {
        sendNum(&client, ERR_CANTKICKSELF, msg.params[0] + " :You cannot kick yourself");
        return ;
    }
    std::string reason = msg.params.size() > 2 ? msg.params[2] : client.getNickname();
    std::string notice = ":" + client.getNickname() + " KICK " + msg.params[0] + " " + target->getNickname() + " :" + reason + "\r\n";
    broadcastToChannel(notice, channel, NULL); // send to everyone including kicked user, before removal
    removeClientFromChannel(target, channel);
}

void Server::handleQuit(Client& client, const Message& msg) {
    std::string quitMessage = msg.params.empty() ? "Client quit" : msg.params[0];
    std::string notice = ":" + client.getNickname() + " QUIT :" + quitMessage + "\r\n";
    broadcastToAllChannels(notice, &client);
    client.setPendingQuit(true);
}

// handle Mode
// parses and validates the MODE command,
// checks for channel existence, client membership, and operator status,
// applies the mode changes to the channel, and broadcasts the mode change message
void Server::handleMode(Client& client, const Message& msg) {
	if (msg.params.empty() || msg.params[0].empty()) {
		sendNum(&client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
		return ;
	}
	
	const std::string& channelName = msg.params[0];
	Channel* channel = findChannelByName(channelName);
	if (!channel) {
		sendNum(&client, ERR_NOSUCHCHANNEL, channelName + " :No such channel");
		return ;
	}

	// MODE command with only channel name returns the current modes of the channel
	if (msg.params.size() == 1) {
		sendNum(&client, RPL_CHANNELMODEIS, channelName + " " + channel->getModes());
		return ;
	}
	if (!channel->hasClient(&client)) {
		sendNum(&client, ERR_NOTONCHANNEL, channelName + " :You're not on that channel");
		return ;
	}
	if (!channel->isOperator(&client)) {
		sendNum(&client, ERR_CHANOPRIVSNEEDED, channelName + " :You're not channel operator");
		return ;
	}

	const std::string& modeChanges = msg.params[1];
	size_t argIndex = 2; // Start from the third parameter for mode arguments

	if (modeChanges.empty() || (modeChanges.size() == 1 && (modeChanges[0] == '+' || modeChanges[0] == '-'))) {
		sendNum(&client, ERR_UNKNOWNMODE, "* :Empty or invalid mode string");
		return ;
	}
	std::string appliedModes;
	std::vector<std::string> modeArgs;

	Client* target = NULL;
	char sign = '+';

	for (size_t i = 0; i < modeChanges.length(); ++i) {
		char modeChar = modeChanges[i];
		if (modeChar == '+' || modeChar == '-') {
			sign = modeChar;
			continue ;
		}

		switch (modeChar) {
			case 'i':
				channel->setInviteOnly(sign == '+');
				appliedModes += sign;
				appliedModes += modeChar;
				break ;
			case 'k':
				if (sign == '+') {
					if (argIndex >= msg.params.size()) {
						sendNum(&client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
						break ;
					}
					channel->setKey(msg.params[argIndex]);
					modeArgs.push_back(msg.params[argIndex++]);
				} else
					channel->removeKey();
				appliedModes += sign;
				appliedModes += modeChar;
				break ;
			case 'l':
				if (sign == '+') {
					if (argIndex >= msg.params.size()) {
						sendNum(&client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
						break ;
					}
					int limit = std::atoi(msg.params[argIndex].c_str());
					if (limit <= 0) {
						sendNum(&client, ERR_UNKNOWNMODE, "l :Invalid limit value");
						argIndex++;
						break ;
					}
					channel->setUserLimit(static_cast<size_t>(limit));
					modeArgs.push_back(msg.params[argIndex++]);
				} else
					channel->removeUserLimit();
				appliedModes += sign;
				appliedModes += modeChar;
				break ;
			case 't':
				channel->setTopicRestriction(sign == '+');
				appliedModes += sign;
				appliedModes += modeChar;
				break ;
			case 'o':
				if (argIndex >= msg.params.size()) {
					sendNum(&client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
					break ;
				}
				target = findClientByNickname(msg.params[argIndex]);
				if (!target || !channel->hasClient(target)) {
					sendNum(&client, ERR_USERNOTINCHANNEL, msg.params[argIndex] + " " + channelName + " :They aren't on that channel");
					argIndex++;
					break ;
				}
				if (sign == '+')
					channel->addOperator(target);
				else
					channel->removeOperator(target);
				modeArgs.push_back(msg.params[argIndex]);
				appliedModes += sign;
				appliedModes += modeChar;
                argIndex++;
				break ;
			default:
				sendNum(&client, ERR_UNKNOWNMODE, std::string(1, modeChar) + " :is unknown mode char to me");
				return ;
		}
	}

	if (appliedModes.empty())
		return ;

	std::string broadcastMsg = ":" + client.getNickname() + "!" + client.getUsername() 
								+ "@" + client.getHostname() + " MODE " + channelName + " " + appliedModes;
    std::cout << "[DEBUG] Broadcast message: " << broadcastMsg << std::endl;
    std::cout << "[DEBUG] Mode arguments size: " << modeArgs.size() << std::endl;
    std::cout << "[DEBUG] ArgIndex: " << argIndex << std::endl;
	for (size_t i = 0; i < modeArgs.size(); ++i){
		broadcastMsg += " " + modeArgs[i]; // Append mode arguments in order
        std::cout << "[DEBUG] Mode argument: " << modeArgs[i] << std::endl;
    }
	broadcastMsg += "\r\n";
	broadcastToChannel(broadcastMsg, channel, NULL);
}
