#include <sys/epoll.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>

int main()
{
    constexpr int MAX_EVENTS = 10;

    // 1. Create epoll instance
    int epollFd = epoll_create1(0);

    if (epollFd == -1)
    {
        std::cerr << "epoll_create1 failed: "
                  << std::strerror(errno)
                  << '\n';

        return 1;
    }

    // 2. Configure the event
    struct epoll_event event{};

    event.events = EPOLLIN;
    event.data.fd = STDIN_FILENO;

    // 3. Add stdin to epoll
    if (epoll_ctl(
            epollFd,
            EPOLL_CTL_ADD,
            STDIN_FILENO,
            &event) == -1)
    {
        std::cerr << "epoll_ctl failed: "
                  << std::strerror(errno)
                  << '\n';

        close(epollFd);
        return 1;
    }

    // 4. Storage for ready events
    struct epoll_event events[MAX_EVENTS];

    while (true)
    {
        // 5. Wait for events
        int count = epoll_wait(
            epollFd,
            events,
            MAX_EVENTS,
            -1);

        if (count == -1)
        {
            if (errno == EINTR)
                continue;

            std::cerr << "epoll_wait failed: "
                      << std::strerror(errno)
                      << '\n';

            break;
        }

        // 6. Process ready events
        for (int i = 0; i < count; ++i)
        {
            int fd = events[i].data.fd;

            if (fd == STDIN_FILENO)
            {
                std::cout << "EPOLLIN: stdin is ready!\n";

                char buffer[1024];

                ssize_t bytesRead = read(
                    STDIN_FILENO,
                    buffer,
                    sizeof(buffer) - 1);

                if (bytesRead > 0)
                {
                    buffer[bytesRead] = '\0';

                    std::cout
                        << "Received: "
                        << buffer;
                }
            }
        }
    }

    close(epollFd);

    return 0;
}
