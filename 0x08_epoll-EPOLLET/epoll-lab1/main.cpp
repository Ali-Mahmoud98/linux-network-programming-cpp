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
constexpr int BUFFER_SIZE = 1024;

bool setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
        return false;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

int main()
{
    // --------------------------------------------------
    // 1. Create server socket
    // --------------------------------------------------

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFd == -1)
    {
        std::cerr << "socket failed: "
                  << std::strerror(errno) << '\n';

        return 1;
    }

    if (!setNonBlocking(serverFd))
    {
        std::cerr << "setNonBlocking failed\n";
        close(serverFd);
        return 1;
    }

    // --------------------------------------------------
    // 2. bind()
    // --------------------------------------------------

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
                  << std::strerror(errno) << '\n';

        close(serverFd);
        return 1;
    }

    // --------------------------------------------------
    // 3. listen()
    // --------------------------------------------------

    if (listen(serverFd, SOMAXCONN) == -1)
    {
        std::cerr << "listen failed: "
                  << std::strerror(errno) << '\n';

        close(serverFd);
        return 1;
    }

    // --------------------------------------------------
    // 4. Create epoll instance
    // --------------------------------------------------

    int epollFd = epoll_create1(0);

    if (epollFd == -1)
    {
        std::cerr << "epoll_create1 failed\n";

        close(serverFd);
        return 1;
    }

    // --------------------------------------------------
    // 5. Add serverFd to epoll
    // --------------------------------------------------

    epoll_event serverEvent{};

    serverEvent.events = EPOLLIN | EPOLLET;
    serverEvent.data.fd = serverFd;

    if (epoll_ctl(
            epollFd,
            EPOLL_CTL_ADD,
            serverFd,
            &serverEvent) == -1)
    {
        std::cerr << "epoll_ctl ADD server failed\n";

        close(epollFd);
        close(serverFd);
        return 1;
    }

    std::cout << "Server listening on port "
              << PORT << '\n';

    // --------------------------------------------------
    // 6. Event loop
    // --------------------------------------------------

    epoll_event events[MAX_EVENTS];

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

            std::cerr << "epoll_wait failed\n";
            break;
        }

        // --------------------------------------------------
        // Process ready events
        // --------------------------------------------------

        for (int i = 0; i < count; ++i)
        {
            int fd = events[i].data.fd;

            // ==========================================
            // Listening socket
            // ==========================================

            if (fd == serverFd)
            {
                while (true)
                {
                    int clientFd =
                        accept(serverFd, nullptr, nullptr);

                    if (clientFd == -1)
                    {
                        if (errno == EAGAIN ||
                            errno == EWOULDBLOCK)
                        {
                            // No more pending connections
                            break;
                        }

                        std::cerr
                            << "accept failed: "
                            << std::strerror(errno)
                            << '\n';

                        break;
                    }

                    std::cout
                        << "New client: "
                        << clientFd
                        << '\n';

                    // Make client socket non-blocking
                    if (!setNonBlocking(clientFd))
                    {
                        close(clientFd);
                        continue;
                    }

                    // Add client socket to epoll
                    epoll_event clientEvent{};

                    clientEvent.events =
                        EPOLLIN | EPOLLET;

                    clientEvent.data.fd = clientFd;

                    if (epoll_ctl(
                            epollFd,
                            EPOLL_CTL_ADD,
                            clientFd,
                            &clientEvent) == -1)
                    {
                        std::cerr
                            << "epoll_ctl ADD client failed\n";

                        close(clientFd);
                    }
                }

                continue;
            }

            // ==========================================
            // Client socket
            // ==========================================

            if (events[i].events & EPOLLIN)
            {
                while (true)
                {
                    char buffer[BUFFER_SIZE];

                    ssize_t bytesRead =
                        recv(
                            fd,
                            buffer,
                            sizeof(buffer),
                            0);

                    // ----------------------------------
                    // Data received
                    // ----------------------------------

                    if (bytesRead > 0)
                    {
                        std::cout
                            << "Client "
                            << fd
                            << " sent "
                            << bytesRead
                            << " bytes:\n";

                        std::cout.write(
                            buffer,
                            bytesRead);

                        std::cout << '\n';

                        // IMPORTANT:
                        // Continue reading.
                        continue;
                    }

                    // ----------------------------------
                    // Connection closed
                    // ----------------------------------

                    if (bytesRead == 0)
                    {
                        std::cout
                            << "Client "
                            << fd
                            << " disconnected\n";

                        epoll_ctl(
                            epollFd,
                            EPOLL_CTL_DEL,
                            fd,
                            nullptr);

                        close(fd);

                        break;
                    }

                    // ----------------------------------
                    // No more data
                    // ----------------------------------

                    if (errno == EAGAIN ||
                        errno == EWOULDBLOCK)
                    {
                        // We drained the socket.
                        break;
                    }

                    // ----------------------------------
                    // Real error
                    // ----------------------------------

                    std::cerr
                        << "recv failed: "
                        << std::strerror(errno)
                        << '\n';

                    epoll_ctl(
                        epollFd,
                        EPOLL_CTL_DEL,
                        fd,
                        nullptr);

                    close(fd);

                    break;
                }
            }
        }
    }

    close(epollFd);
    close(serverFd);

    return 0;
}
