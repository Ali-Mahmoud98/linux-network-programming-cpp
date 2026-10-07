#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

constexpr int PORT = 12345;
constexpr int MAX_EVENTS = 10;

bool setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
        return false;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

int main()
{
    // --------------------------------
    // 1. Create listening socket
    // --------------------------------

    int serverFd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (serverFd == -1)
    {
        std::cerr << "socket(): "
                  << std::strerror(errno)
                  << '\n';

        return 1;
    }

    // --------------------------------
    // 2. Make it non-blocking
    // --------------------------------

    if (!setNonBlocking(serverFd))
    {
        std::cerr << "setNonBlocking(): "
                  << std::strerror(errno)
                  << '\n';

        close(serverFd);
        return 1;
    }

    // --------------------------------
    // 3. bind()
    // --------------------------------

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(
            serverFd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == -1)
    {
        std::cerr << "bind(): "
                  << std::strerror(errno)
                  << '\n';

        close(serverFd);
        return 1;
    }

    // --------------------------------
    // 4. listen()
    // --------------------------------

    if (listen(serverFd, SOMAXCONN) == -1)
    {
        std::cerr << "listen(): "
                  << std::strerror(errno)
                  << '\n';

        close(serverFd);
        return 1;
    }

    std::cout
        << "Server listening on port "
        << PORT
        << '\n';

    // --------------------------------
    // 5. Create epoll instance
    // --------------------------------

    int epollFd = epoll_create1(0);

    if (epollFd == -1)
    {
        std::cerr << "epoll_create1(): "
                  << std::strerror(errno)
                  << '\n';

        close(serverFd);
        return 1;
    }

    // --------------------------------
    // 6. Add serverFd to epoll
    // --------------------------------

    epoll_event serverEvent{};

    serverEvent.events = EPOLLIN;
    serverEvent.data.fd = serverFd;

    if (epoll_ctl(
            epollFd,
            EPOLL_CTL_ADD,
            serverFd,
            &serverEvent) == -1)
    {
        std::cerr << "epoll_ctl(): "
                  << std::strerror(errno)
                  << '\n';

        close(epollFd);
        close(serverFd);
        return 1;
    }

    // --------------------------------
    // 7. Event array
    // --------------------------------

    epoll_event events[MAX_EVENTS];

    // --------------------------------
    // 8. Event loop
    // --------------------------------

    while (true)
    {
        int count = epoll_wait(
            epollFd,
            events,
            MAX_EVENTS,
            -1
        );

        if (count == -1)
        {
            if (errno == EINTR)
                continue;

            std::cerr << "epoll_wait(): "
                      << std::strerror(errno)
                      << '\n';

            break;
        }

        // --------------------------------
        // Process ready events
        // --------------------------------

        for (int i = 0; i < count; ++i)
        {
            int fd = events[i].data.fd;

            // --------------------------------
            // New connection
            // --------------------------------

            if (fd == serverFd)
            {
                int clientFd = accept(
                    serverFd,
                    nullptr,
                    nullptr
                );

                if (clientFd == -1)
                {
                    if (errno == EAGAIN ||
                        errno == EWOULDBLOCK)
                    {
                        continue;
                    }

                    std::cerr << "accept(): "
                              << std::strerror(errno)
                              << '\n';

                    continue;
                }

                // Make client socket non-blocking
                if (!setNonBlocking(clientFd))
                {
                    close(clientFd);
                    continue;
                }

                std::cout
                    << "New client: fd="
                    << clientFd
                    << '\n';

                // Add client to epoll
                epoll_event clientEvent{};

                clientEvent.events = EPOLLIN;
                clientEvent.data.fd = clientFd;

                if (epoll_ctl(
                        epollFd,
                        EPOLL_CTL_ADD,
                        clientFd,
                        &clientEvent) == -1)
                {
                    std::cerr
                        << "epoll_ctl(ADD client): "
                        << std::strerror(errno)
                        << '\n';

                    close(clientFd);
                }

                continue;
            }

            // --------------------------------
            // Existing client
            // --------------------------------

            if (events[i].events & EPOLLIN)
            {
                char buffer[1024];

                ssize_t bytesRead = recv(
                    fd,
                    buffer,
                    sizeof(buffer) - 1,
                    0
                );

                if (bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';

                    std::cout
                        << "Client "
                        << fd
                        << ": "
                        << buffer;
                }
                else if (bytesRead == 0)
                {
                    std::cout
                        << "Client "
                        << fd
                        << " disconnected\n";

                    epoll_ctl(
                        epollFd,
                        EPOLL_CTL_DEL,
                        fd,
                        nullptr
                    );

                    close(fd);
                }
                else
                {
                    if (errno != EAGAIN &&
                        errno != EWOULDBLOCK)
                    {
                        std::cerr
                            << "recv(): "
                            << std::strerror(errno)
                            << '\n';

                        epoll_ctl(
                            epollFd,
                            EPOLL_CTL_DEL,
                            fd,
                            nullptr
                        );

                        close(fd);
                    }
                }
            }
        }
    }

    close(epollFd);
    close(serverFd);

    return 0;
}
