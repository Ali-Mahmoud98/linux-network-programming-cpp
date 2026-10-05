#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main()
{
    int server_fd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (server_fd == -1)
    {
        perror("socket");
        return 1;
    }

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(12345);

    if (bind(
            server_fd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == -1)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, SOMAXCONN) == -1)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    std::cout << "Server listening on port 12345\n";
    std::cout << "Calling accept()...\n";

    int client_fd = accept(
        server_fd,
        nullptr,
        nullptr
    );

    if (client_fd == -1)
    {
        perror("accept");
        close(server_fd);
        return 1;
    }

    std::cout << "Client connected!\n";

    close(client_fd);
    close(server_fd);

    return 0;
}
