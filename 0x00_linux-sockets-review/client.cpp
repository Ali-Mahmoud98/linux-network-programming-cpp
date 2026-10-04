#include <iostream>
#include <string>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main()
{
    // 1. Create socket
    int clientFd = socket(AF_INET, SOCK_STREAM, 0);

    if (clientFd == -1)
    {
        std::cerr << "socket() failed\n";
        return 1;
    }

    // 2. URL:
    // http://localhost:8080/

    sockaddr_in serverAddress{};

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);

    inet_pton(
        AF_INET,
        "127.0.0.1",
        &serverAddress.sin_addr
    );

    // 3. Connect to server
    if (connect(
            clientFd,
            reinterpret_cast<sockaddr*>(&serverAddress),
            sizeof(serverAddress)) == -1)
    {
        std::cerr << "connect() failed\n";
        close(clientFd);
        return 1;
    }

    // 4. Create HTTP request
    std::string request =
        "GET / HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Connection: close\r\n"
        "\r\n";

    // 5. Send HTTP request
    send(
        clientFd,
        request.c_str(),
        request.size(),
        0
    );

    // 6. Receive HTTP response
    char buffer[4096];

    int bytesReceived = recv(
        clientFd,
        buffer,
        sizeof(buffer) - 1,
        0
    );

    if (bytesReceived > 0)
    {
        buffer[bytesReceived] = '\0';

        std::cout << buffer;
    }

    close(clientFd);

    return 0;
}
