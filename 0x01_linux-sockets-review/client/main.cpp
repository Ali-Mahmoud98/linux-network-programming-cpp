#include <iostream>
#include <string>

#include "../socket/Socket.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <message>" << std::endl;
        return 1;
    }

    try {
        Socket socket;
        socket.connect("127.0.0.1", 8080);

        // The server splits messages on '\n'.
        std::string message = std::string(argv[1]) + "\n";
        socket.send(message.data(), message.size());
        // Socket destructor closes the connection; the server keeps running.
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
