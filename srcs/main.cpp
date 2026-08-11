#include "../incs/Server.hpp"

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        std::cerr << "Error: invalid port" << std::endl;
        return 1;
    }

    Server server(port, argv[2]);
    server.initServer();
    server.run();

    return 0;
}