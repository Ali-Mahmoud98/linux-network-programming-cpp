#include <iostream>
#include <string>

#include "../socket/Socket.hpp"

int main() {
    Socket socket;

    socket.bind(8080);
    socket.listen(10);
    std::cout << "Listening on port 8080..." << std::endl;

    bool running = true;
    while (running) {
        try {
            Socket client = socket.accept();
            std::cout << "Client connected!" << std::endl;

            std::string pending;   // holds data until a full line arrives
            char buffer[1024];
            bool connected = true;

            while (connected) {
                int n = client.receive(buffer, sizeof(buffer));
                if (n == 0) {
                    std::cout << "Client disconnected." << std::endl;
                    break;
                }
                pending.append(buffer, n);

                // Process every complete line currently in `pending`
                size_t pos;
                while ((pos = pending.find('\n')) != std::string::npos) {
                    std::string line = pending.substr(0, pos);
                    pending.erase(0, pos + 1);
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();            // telnet sends \r\n
                    }

                    if (line == "close") {
                        std::cout << "Close command received. Shutting down." << std::endl;
                        std::string bye = "Server closing.\n";
                        client.send(bye.data(), bye.size());
                        running = false;            // stops the accept loop
                        connected = false;          // stops the client loop
                        break;
                    }

                    std::cout << "Received: " << line << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }

    std::cout << "Server stopped." << std::endl;
    return 0;
}
