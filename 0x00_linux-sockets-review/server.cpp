#include <iostream>
#include <cstring>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main()
{
    // 1. Create socket
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFd == -1)
    {
        std::cerr << "socket() failed\n";
        return 1;
    }

    // 2. Address
    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    // 3. Bind
    if (bind(
            serverFd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == -1)
    {
        std::cerr << "bind() failed\n";
        return 1;
    }

    // 4. Listen
    if (listen(serverFd, 10) == -1)
    {
        std::cerr << "listen() failed\n";
        return 1;
    }

    std::cout << "Server running on http://localhost:8080\n";

    // 5. Accept one client
    int clientFd = accept(serverFd, nullptr, nullptr);

    if (clientFd == -1)
    {
        std::cerr << "accept() failed\n";
        return 1;
    }

    // 6. Read HTTP request
    char buffer[4096];

    recv(clientFd, buffer, sizeof(buffer), 0);

    // 7. HTML
    const char* html =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>My C++ Server</title>"
        "</head>"
        "<body>"
        "<h1>Hello from C++!</h1>"
        "<p>This page was sent using a TCP socket.</p>"
        "</body>"
        "</html>";

    // 8. HTTP response
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(strlen(html)) + "\r\n"
        "\r\n" +
        html;

    // 9. Send response
    send(
        clientFd,
        response.c_str(),
        response.size(),
        0
    );

    // 10. Close
    close(clientFd);
    close(serverFd);

    return 0;
}
