#include "../incs/Message.hpp"

Message parseLine(const std::string& line) {
		Message message;
		size_t pos = 0;

		// Check for prefix
		if (!line.empty() && line[0] == ':') {
			pos = line.find(' ');
			if (pos != std::string::npos) {
				message.prefix = line.substr(1, pos - 1);
				pos++; // Move past the space
			} else {
				// Malformed message, return empty message
				return message;
			}
		}

		// Extract command
		size_t commandEnd = line.find(' ', pos);
		if (commandEnd != std::string::npos) {
			message.command = line.substr(pos, commandEnd - pos);
			pos = commandEnd + 1; // Move past the space
		} else {
			// No parameters, the rest of the line is the command
			message.command = line.substr(pos);
			return message;
		}

		// Extract parameters
		while (pos < line.size()) {
			if (line[pos] == ':') {
				// The rest of the line is a single parameter
				message.params.push_back(line.substr(pos + 1));
				break;
			}

			size_t paramEnd = line.find(' ', pos);
			if (paramEnd != std::string::npos) {
				message.params.push_back(line.substr(pos, paramEnd - pos));
				pos = paramEnd + 1; // Move past the space
			} else {
				// Last parameter
				message.params.push_back(line.substr(pos));
				break;
			}
		}
		return message;
}