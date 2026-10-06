#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <iostream>

constexpr int PORT = 12345;
constexpr int MAX_CLIENTS = 10;

int setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
        return -1;

    return fcntl(
        fd,
        F_SETFL,
        flags | O_NONBLOCK);
}

int main()
{
    // --------------------------------------------------
    // 1. Create socket
    // --------------------------------------------------

    int server_fd = socket(
        AF_INET,
        SOCK_STREAM,
        0);

    if (server_fd == -1)
    {
        perror("socket");
        return 1;
    }

    // --------------------------------------------------
    // 2. SO_REUSEADDR
    // --------------------------------------------------

    int opt = 1;

    setsockopt(
        server_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt));

    // --------------------------------------------------
    // 3. Bind
    // --------------------------------------------------

    sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
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

    setNonBlocking(server_fd);

    std::cout
        << "Server listening on port "
        << PORT
        << '\n';

    // --------------------------------------------------
    // 6. pollfd array
    // --------------------------------------------------

    pollfd fds[MAX_CLIENTS + 1]{};

    // Listening socket
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    // Empty client slots
    for (int i = 1; i <= MAX_CLIENTS; ++i)
    {
        fds[i].fd = -1;
    }

    // --------------------------------------------------
    // 7. Event loop
    // --------------------------------------------------

    while (true)
    {
        int ret = poll(
            fds,
            MAX_CLIENTS + 1,
            -1);

        if (ret == -1)
        {
            if (errno == EINTR)
                continue;

            perror("poll");
            break;
        }

        // --------------------------------------------------
        // 8. Listening socket
        // --------------------------------------------------

        if (fds[0].revents & POLLIN)
        {
            sockaddr_in client_addr{};
            socklen_t client_len =
                sizeof(client_addr);

            int client_fd = accept(
                server_fd,
                reinterpret_cast<sockaddr*>(&client_addr),
                &client_len);

            if (client_fd != -1)
            {
                setNonBlocking(client_fd);

                bool added = false;

                for (int i = 1;
                     i <= MAX_CLIENTS;
                     ++i)
                {
                    if (fds[i].fd == -1)
                    {
                        fds[i].fd = client_fd;
                        fds[i].events = POLLIN;

                        added = true;

                        std::cout
                            << "New client: fd="
                            << client_fd
                            << '\n';

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
        // 9. Clients
        // --------------------------------------------------

        for (int i = 1;
             i <= MAX_CLIENTS;
             ++i)
        {
            if (fds[i].fd == -1)
                continue;

            if (fds[i].revents & POLLIN)
            {
                char buffer[1024];

                ssize_t bytes = recv(
                    fds[i].fd,
                    buffer,
                    sizeof(buffer),
                    0);

                if (bytes > 0)
                {
                    std::cout
                        << "Received "
                        << bytes
                        << " bytes from fd="
                        << fds[i].fd
                        << '\n';

                    send(
                        fds[i].fd,
                        buffer,
                        bytes,
                        0);
                }
                else if (bytes == 0)
                {
                    std::cout
                        << "Client disconnected: fd="
                        << fds[i].fd
                        << '\n';

                    close(fds[i].fd);

                    fds[i].fd = -1;
                }
                else
                {
                    if (errno != EAGAIN &&
                        errno != EWOULDBLOCK)
                    {
                        perror("recv");

                        close(fds[i].fd);

                        fds[i].fd = -1;
                    }
                }
            }
        }
    }

    close(server_fd);

    return 0;
}
