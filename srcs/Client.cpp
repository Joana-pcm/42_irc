#include "../incs/Client.hpp"

Client::Client() {
	_clientFd = -1;
	_connected = false;
	_hasNick = false;
	_hasUser = false;
	_hasPass = false;
	_pendingQuit = false;
}

Client::Client(int fd) {
	_clientFd = fd;
	_connected = true;
	_hasNick = false;
	_hasUser = false;
	_hasPass = false;
	_pendingQuit = false;
// checks for channel existence, client membership, and operator status,
}

Client::~Client()
{
	if (_connected) {
		close(_clientFd);
		_connected = false;
	}
}


// getters
int Client::getFd() const				 { return _clientFd; }
std::string Client::getNickname() const  { return _nickname; }
std::string Client::getUsername() const  { return _username; }
std::string Client::getRealname() const  { return _realname; }
std::string Client::getHostname() const  { return _hostname; }
std::string Client::getOutBuffer() const { return _outBuffer; }

// check status
bool Client::isConnected() const		{ return _connected; }
bool Client::isPendingQuit() const		{ return _pendingQuit; }
bool Client::isAuthenticated() const	{ return _hasNick && _hasUser && _hasPass; }

// setters
void Client::setFd(int fd)								{ _clientFd = fd; }
void Client::setRealname(const std::string& real)		{ _realname = real; }
void Client::setHasPass(bool status)					{ _hasPass = status; }
void Client::setConnected(bool status)					{ _connected = status; }
void Client::setHostname(const std::string& hostname)	{ _hostname = hostname; }
void Client::setPendingQuit(bool status)				{ _pendingQuit = status; }
void Client::setNickname(const std::string& nick) {
	_hasNick = true;
	_nickname = nick;
}
void Client::setUsername(const std::string& user) {
	_hasUser = true;
	_username = user;
}

// buffer management
void Client::appendToInBuffer(const char* data, size_t len) {
	_inBuffer.append(data, len);
}
void Client::queueOutgoing(const std::string& data) {
	_outBuffer.append(data);
}
void Client::clearOutBuffer(size_t bytesSent) {
	if (bytesSent >= _outBuffer.size()) {
		_outBuffer.clear();
	} else {
		_outBuffer.erase(0, bytesSent);
	}
}

std::vector<std::string> Client::extractCompleteLines() {
	std::vector<std::string> lines;
	size_t pos = 0;
	while ((pos = _inBuffer.find("\r\n")) != std::string::npos) {
		lines.push_back(_inBuffer.substr(0, pos));
		_inBuffer.erase(0, pos + 2); // Remove the line and the \r\n
	}
	return lines;
}