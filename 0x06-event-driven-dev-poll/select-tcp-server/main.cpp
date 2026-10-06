#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

constexpr int PORT = 12345;
constexpr int MAX_CLIENTS = 10;

int setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
        return -1;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main()
{
    // --------------------------------------------------
    // 1. Create listening socket
    // --------------------------------------------------

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1)
    {
        perror("socket");
        return 1;
    }

    // --------------------------------------------------
    // 2. Allow address reuse
    // --------------------------------------------------

    int opt = 1;

    if (setsockopt(
            server_fd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &opt,
            sizeof(opt)) == -1)
    {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    // --------------------------------------------------
    // 3. Bind
    // --------------------------------------------------

    sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // listen on all interfaces: 0.0.0.0
    server_addr.sin_port = htons(PORT);

    if (bind(
            server_fd,
            reinterpret_cast<sockaddr*>(&server_addr),
            sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    // --------------------------------------------------
    // 4. Listen
    // --------------------------------------------------

    if (listen(server_fd, 10) == -1)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    // --------------------------------------------------
    // 5. Non-blocking
    // --------------------------------------------------

    if (setNonBlocking(server_fd) == -1)
    {
        perror("fcntl");
        close(server_fd);
        return 1;
    }

    std::cout << "Server listening on port "
              << PORT << '\n';

    // --------------------------------------------------
    // 6. Client sockets
    // --------------------------------------------------

    int clients[MAX_CLIENTS];

    for (int& client : clients)
    {
        client = -1;
    }

    // --------------------------------------------------
    // 7. Event loop
    // --------------------------------------------------

    while (true)
    {
        fd_set readfds;

        FD_ZERO(&readfds);

        // Add listening socket
        FD_SET(server_fd, &readfds);

        int max_fd = server_fd;

        // Add client sockets
        for (int client_fd : clients)
        {
            if (client_fd != -1)
            {
                FD_SET(client_fd, &readfds);

                if (client_fd > max_fd)
                {
                    max_fd = client_fd;
                }
            }
        }

        // --------------------------------------------------
        // 8. Wait for events
        // --------------------------------------------------

        int ret = select(
            max_fd + 1,
            &readfds,
            nullptr,
            nullptr,
            nullptr); // select call to wait for events
            // block until one of the file descriptors is readable

        if (ret == -1)
        {
            perror("select");
            break;
        }

        // --------------------------------------------------
        // 9. Check listening socket
        // --------------------------------------------------

        if (FD_ISSET(server_fd, &readfds))
        {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);

            int client_fd = accept(
                server_fd,
                reinterpret_cast<sockaddr*>(&client_addr),
                &client_len);

            if (client_fd != -1)
            {
                setNonBlocking(client_fd);

                std::cout
                    << "New client: fd="
                    << client_fd
                    << '\n';

                bool added = false;

                for (int& slot : clients)
                {
                    if (slot == -1)
                    {
                        slot = client_fd;
                        added = true;
                        break;
                    }
                }

                if (!added)
                {
                    std::cout
                        << "Too many clients\n";

                    close(client_fd);
                }
            }
        }

        // --------------------------------------------------
        // 10. Check clients
        // --------------------------------------------------

        for (int& client_fd : clients)
        {
            if (client_fd == -1)
                continue;

            if (FD_ISSET(client_fd, &readfds))
            {
                char buffer[1024];

                ssize_t bytes = recv(
                    client_fd,
                    buffer,
                    sizeof(buffer),
                    0);

                if (bytes > 0)
                {
                    std::cout
                        << "Received "
                        << bytes
                        << " bytes from fd="
                        << client_fd
                        << '\n';

                    send(
                        client_fd,
                        buffer,
                        bytes,
                        0);
                }
                else if (bytes == 0)
                {
                    std::cout
                        << "Client disconnected: fd="
                        << client_fd
                        << '\n';

                    close(client_fd);
                    client_fd = -1;
                }
                else
                {
                    if (errno != EAGAIN &&
                        errno != EWOULDBLOCK)
                    {
                        perror("recv");

                        close(client_fd);
                        client_fd = -1;
                    }
                }
            }
        }
    }

    close(server_fd);

    return 0;
}
