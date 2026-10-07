#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

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
    // ============================================
    // Server socket
    // ============================================

    int serverFd = socket(
        AF_INET,
        SOCK_STREAM,
        0);

    if (serverFd == -1)
    {
        std::cerr << "socket failed\n";
        return 1;
    }

    setNonBlocking(serverFd);

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(
            serverFd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == -1)
    {
        std::cerr << "bind failed: "
                  << std::strerror(errno)
                  << '\n';

        close(serverFd);
        return 1;
    }

    if (listen(serverFd, SOMAXCONN) == -1)
    {
        std::cerr << "listen failed\n";

        close(serverFd);
        return 1;
    }

    // ============================================
    // epoll
    // ============================================

    int epollFd = epoll_create1(0);

    if (epollFd == -1)
    {
        std::cerr << "epoll_create1 failed\n";

        close(serverFd);
        return 1;
    }

    epoll_event serverEvent{};

    serverEvent.events = EPOLLIN;
    serverEvent.data.fd = serverFd;

    epoll_ctl(
        epollFd,
        EPOLL_CTL_ADD,
        serverFd,
        &serverEvent);

    // ============================================
    // Prepare BIG message
    // ============================================

    std::string message(
        10 * 1024 * 1024,
        'A');

    std::cout
        << "Prepared "
        << message.size()
        << " bytes\n";

    epoll_event events[MAX_EVENTS];

    // ============================================
    // Event loop
    // ============================================

    while (true)
    {
        int count = epoll_wait(
            epollFd,
            events,
            MAX_EVENTS,
            -1);

        if (count == -1)
        {
            if (errno == EINTR)
                continue;

            break;
        }

        for (int i = 0; i < count; ++i)
        {
            int fd = events[i].data.fd;

            // ====================================
            // New connection
            // ====================================

            if (fd == serverFd)
            {
                int clientFd =
                    accept(
                        serverFd,
                        nullptr,
                        nullptr);

                if (clientFd == -1)
                    continue;

                std::cout
                    << "Client connected. fd = "
                    << clientFd
                    << '\n';

                setNonBlocking(clientFd);

                epoll_event clientEvent{};

                clientEvent.events =
                    EPOLLOUT;

                clientEvent.data.fd =
                    clientFd;

                epoll_ctl(
                    epollFd,
                    EPOLL_CTL_ADD,
                    clientFd,
                    &clientEvent);

                continue;
            }

            // ====================================
            // Writable
            // ====================================

            if (events[i].events & EPOLLOUT)
            {
                std::cout
                    << "EPOLLOUT\n";

                ssize_t bytesSent =
                    send(
                        fd,
                        message.data(),
                        message.size(),
                        0);

                if (bytesSent == -1)
                {
                    if (errno == EAGAIN ||
                        errno == EWOULDBLOCK)
                    {
                        std::cout
                            << "send() -> EAGAIN\n";
                    }
                    else
                    {
                        std::cerr
                            << "send() failed: "
                            << std::strerror(errno)
                            << '\n';
                    }

                    continue;
                }

                std::cout
                    << "send() -> "
                    << bytesSent
                    << " bytes\n";
            }
        }
    }

    close(epollFd);
    close(serverFd);
}
