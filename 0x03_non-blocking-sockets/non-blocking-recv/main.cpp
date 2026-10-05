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

    // START OF ACCEPTING CLIENT CONNECTIONS IN NON-BLOCKING MODE
    // the following part is the update of blocking socket lab.
    while (true)
    {
        int client_fd = accept(
            server_fd,
            nullptr,
            nullptr
        );

        if (client_fd == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::cout << "No client yet...\n";
                sleep(1);
                continue;
            }

            perror("accept");
            break;
        }

        // adding the part that makes the recv() call non-blocking.
        // We will make the client fd non-blocking.
        // Start
        int client_flags = fcntl(client_fd, F_GETFL);
        if (client_flags == -1)
        {
            perror("F_GETFL client");
            close(client_fd);
            continue;
        }

        if (fcntl(
                client_fd,
                F_SETFL,
                client_flags | O_NONBLOCK) == -1)
        {
            perror("F_SETFL client");
            close(client_fd);
            continue;
        }
        // End

        std::cout << "Client connected!\n";

        // The added part for receiving data from the client
        // this is the START of the updates added to non-blocking-accept/main2.cpp.
        char buffer[1024];

        std::cout << "Calling recv()...\n";

        ssize_t n = recv(
            client_fd,
            buffer,
            sizeof(buffer),
            0
        );

        if (n == -1)
        {
            perror("recv");
        }
        else if (n == 0)
        {
            std::cout << "Client closed connection\n";
        }
        else
        {
            std::cout << "Received "
                    << n
                    << " bytes\n";

            std::cout.write(buffer, n);
            std::cout << '\n';
        }
        // this is the END of the updates added to non-blocking-accept/main2.cpp.

        close(client_fd);
    }
    // END OF ACCEPTING CLIENT CONNECTIONS IN NON-BLOCKING MODE
    
    close(server_fd);

    return 0;
}
