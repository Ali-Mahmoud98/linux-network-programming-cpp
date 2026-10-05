#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>

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

    // START OF NON-BLOCKING MODE
    // the following part is the update of blocking socket lab.
    int flags = fcntl(server_fd, F_GETFL);

    if (flags == -1)
    {
        perror("F_GETFL");
        close(server_fd);
        return 1;
    }

    if (fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("F_SETFL");
        close(server_fd);
        return 1;
    }
    
    // END OF NON-BLOCKING MODE

    std::cout << "Server listening on port 12345\n";
    std::cout << "Calling accept()...\n";

    int client_fd = accept(
        server_fd,
        nullptr,
        nullptr
    );

    // START OF ACCEPTING CLIENT CONNECTIONS IN NON-BLOCKING MODE
    // the following part is the update of blocking socket lab.
    if (client_fd == -1)
    {
        std::cout << "accept() returned -1\n";
        std::cout << "errno: " << errno << "\n";

        perror("accept");
    } else
    {
        std::cout << "Client connected with fd " << client_fd << "!\n";
        close(client_fd);
    }
    // END OF ACCEPTING CLIENT CONNECTIONS IN NON-BLOCKING MODE
    
    close(server_fd);

    return 0;
}
