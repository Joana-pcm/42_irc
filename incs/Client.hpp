#pragma once

#include "Server.hpp"
#include "Message.hpp"

#define BUFFER_SIZE 1024

class Client
{
	private:
		int 		_clientFd;
		bool 		_connected;
		bool 		_hasUser;
		bool 		_hasPass;
		bool 		_hasNick;
		bool 		_pendingQuit;
		std::string _username;
		std::string _nickname;
		std::string _realname;
		std::string _hostname;
		std::string _inBuffer; // buffer for incoming data
		std::string _outBuffer; // buffer for outgoing data
		
		public:
		// constructors
		Client();
		Client(int fd);
		~Client();
		Client(const Client& other);
		
		// getters
		int getFd() const;
		std::string getNickname() const;
		std::string getUsername() const;
		std::string getRealname() const;
		std::string getHostname() const;
		std::string getOutBuffer() const;

		// check status
		bool isAuthenticated() const;
		bool isConnected() const;
		bool isPendingQuit() const;
		
		// setters
		void setFd(int fd);
		void setAuthenticated(bool status);
		void setConnected(bool status);
		void setPendingQuit(bool status);
		void setHasPass(bool status);
		void setNickname(const std::string& nick);
		void setUsername(const std::string& user);
		void setRealname(const std::string& real);
		void setHostname(const std::string& hostname);

		// buffer management
		void appendToInBuffer(const char* data, size_t len);
        std::vector<std::string> extractCompleteLines(); // pulls \r\n-terminated lines out
        void queueOutgoing(const std::string& data);
		void clearOutBuffer(size_t bytesSent);
};