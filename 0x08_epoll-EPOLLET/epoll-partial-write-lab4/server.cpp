#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>

constexpr int PORT = 12345;
constexpr int MAX_EVENTS = 10;
constexpr std::size_t MESSAGE_SIZE = 10 * 1024 * 1024;

struct ClientState
{
    std::size_t outputOffset = 0;   // bytes of the message already accepted by send()
    int wakeups = 0;                // EPOLLOUT events handled
    int sendCalls = 0;              // send() calls that accepted data
    int eagainCount = 0;            // times send() returned EAGAIN
};

bool setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
        return false;

    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

void closeClient(
    int epollFd,
    std::unordered_map<int, ClientState>& clients,
    int fd)
{
    epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    clients.erase(fd);
}

int main()
{
    // ============================================
    // Server socket
    // ============================================

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if (serverFd == -1)
    {
        std::cerr << "socket failed\n";
        return 1;
    }

    int reuse = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

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

    epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &serverEvent);

    // ============================================
    // The message: byte i is 'A' + (i % 26), so the
    // client can detect missing or repeated data.
    // ============================================

    std::string message(MESSAGE_SIZE, 'A');

    for (std::size_t i = 0; i < message.size(); ++i)
        message[i] = static_cast<char>('A' + i % 26);

    std::cout << "Prepared " << message.size() << " bytes\n";

    std::unordered_map<int, ClientState> clients;

    epoll_event events[MAX_EVENTS];

    // ============================================
    // Event loop
    // ============================================

    while (true)
    {
        int count = epoll_wait(epollFd, events, MAX_EVENTS, -1);

        if (count == -1)
        {
            if (errno == EINTR)
                continue;

            break;
        }

        for (int i = 0; i < count; ++i)
        {
            int fd = events[i].data.fd;
            uint32_t ready = events[i].events;

            // ====================================
            // New connections
            // ====================================

            if (fd == serverFd)
            {
                while (true)
                {
                    int clientFd = accept(serverFd, nullptr, nullptr);

                    if (clientFd == -1)
                        break;

                    setNonBlocking(clientFd);

                    epoll_event clientEvent{};

                    clientEvent.events = EPOLLOUT;
                    clientEvent.data.fd = clientFd;

                    epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &clientEvent);

                    clients[clientFd] = ClientState{};

                    std::cout << "Client connected. fd = " << clientFd << '\n';
                }

                continue;
            }

            auto it = clients.find(fd);

            if (it == clients.end())
                continue;

            ClientState& client = it->second;

            if (ready & (EPOLLERR | EPOLLHUP))
            {
                std::cout << "fd " << fd << ": EPOLLERR/EPOLLHUP, closing\n";
                closeClient(epollFd, clients, fd);
                continue;
            }

            // ====================================
            // Writable: continue from outputOffset
            // ====================================

            if (ready & EPOLLOUT)
            {
                ++client.wakeups;

                std::size_t startOffset = client.outputOffset;
                int calls = 0;
                bool failed = false;
                bool blocked = false;

                while (client.outputOffset < message.size())
                {
                    ssize_t sent = send(
                        fd,
                        message.data() + client.outputOffset,
                        message.size() - client.outputOffset,
                        MSG_NOSIGNAL);

                    if (sent > 0)
                    {
                        client.outputOffset += sent;
                        ++calls;
                        continue;
                    }

                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        blocked = true;
                        ++client.eagainCount;
                    }
                    else
                    {
                        std::cerr << "send failed: "
                                  << std::strerror(errno)
                                  << '\n';
                        failed = true;
                    }

                    break;
                }

                if (failed)
                {
                    closeClient(epollFd, clients, fd);
                    continue;
                }

                client.sendCalls += calls;

                std::cout << "fd " << fd << ": EPOLLOUT #" << client.wakeups
                          << ": " << calls << " send() calls accepted "
                          << (client.outputOffset - startOffset)
                          << " bytes, offset = "
                          << client.outputOffset << " / "
                          << message.size() << '\n';

                if (blocked)
                {
                    std::cout << "  send() -> EAGAIN, wait for the next EPOLLOUT\n";
                }
                else
                {
                    std::cout << "All bytes sent. EPOLLOUT wakeups: "
                              << client.wakeups
                              << ", send() calls: " << client.sendCalls
                              << ", EAGAIN: " << client.eagainCount
                              << '\n';

                    // Nothing left to write: stop watching EPOLLOUT.
                    epoll_event clientEvent{};

                    clientEvent.events = EPOLLIN;
                    clientEvent.data.fd = fd;

                    epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &clientEvent);
                }
            }

            // ====================================
            // Readable: only used to see the client close
            // ====================================

            if (ready & EPOLLIN)
            {
                char buffer[1024];

                ssize_t n = recv(fd, buffer, sizeof(buffer), 0);

                if (n == 0 ||
                    (n == -1 && errno != EAGAIN && errno != EWOULDBLOCK))
                {
                    std::cout << "Client closed, fd = " << fd << '\n';
                    closeClient(epollFd, clients, fd);
                }
            }
        }
    }

    close(epollFd);
    close(serverFd);
}
