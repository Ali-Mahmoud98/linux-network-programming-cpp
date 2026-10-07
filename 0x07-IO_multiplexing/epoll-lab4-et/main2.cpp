// EPOLLET experiment 2: recv() runs in a loop until EAGAIN.
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

constexpr int PORT = 12345;

// Same tiny buffer as main.cpp, so the only difference is the loop.
constexpr std::size_t BUFFER_SIZE = 4;

bool setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
        return false;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

int main()
{
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFd == -1)
    {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(serverFd,
             reinterpret_cast<sockaddr*>(&address),
             sizeof(address)) == -1 ||
        listen(serverFd, SOMAXCONN) == -1 ||
        !setNonBlocking(serverFd))
    {
        perror("server setup");
        close(serverFd);
        return 1;
    }

    int epollFd = epoll_create1(0);

    if (epollFd == -1)
    {
        perror("epoll_create1");
        close(serverFd);
        return 1;
    }

    // The listening socket stays level-triggered.
    epoll_event serverEvent{};
    serverEvent.events = EPOLLIN;
    serverEvent.data.fd = serverFd;

    epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &serverEvent);

    std::cout << "Server listening on port " << PORT
              << " (EPOLLET, recv in a LOOP, buffer = "
              << BUFFER_SIZE << " bytes)\n";

    epoll_event events[10];
    int wakeups = 0;

    while (true)
    {
        int count = epoll_wait(epollFd, events, 10, -1);

        if (count == -1)
        {
            if (errno == EINTR)
                continue;

            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < count; ++i)
        {
            int fd = events[i].data.fd;

            if (fd == serverFd)
            {
                int clientFd = accept(serverFd, nullptr, nullptr);

                if (clientFd == -1)
                    continue;

                setNonBlocking(clientFd);

                // Edge-triggered: we are told only when NEW data arrives.
                epoll_event clientEvent{};
                clientEvent.events = EPOLLIN | EPOLLET;
                clientEvent.data.fd = clientFd;

                epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent);

                std::cout << "New client: fd=" << clientFd << '\n';
                continue;
            }

            std::cout << "[epoll_wait wakeup #" << ++wakeups
                      << "] fd=" << fd << " is readable\n";

            // Drain the socket: with EPOLLET we may not get another event.
            while (true)
            {
                char buffer[BUFFER_SIZE];

                ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

                if (n > 0)
                {
                    std::cout << "  recv() -> " << n << " bytes: \"";
                    std::cout.write(buffer, n);
                    std::cout << "\"\n";
                    continue;
                }

                if (n == 0)
                {
                    std::cout << "  client closed fd=" << fd << '\n';
                    close(fd);
                    break;
                }

                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    std::cout << "  recv() -> EAGAIN (drained)\n";
                    break;
                }

                perror("recv");
                close(fd);
                break;
            }
        }
    }

    close(epollFd);
    close(serverFd);

    return 0;
}
