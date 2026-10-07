#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <unordered_map>

constexpr int PORT = 12345;
constexpr int MAX_EVENTS = 10;
constexpr int BUFFER_SIZE = 1024;


// ============================================================
// ClientConnection
// ============================================================

struct ClientConnection
{
    int fd;

    // Data waiting to be sent to the client
    std::string outputBuffer;

    // Number of bytes already sent from outputBuffer
    std::size_t outputOffset = 0;
};


// ============================================================
// Set socket to non-blocking
// ============================================================

bool setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1)
    {
        std::cerr << "fcntl(F_GETFL) failed: "
                  << std::strerror(errno) << '\n';

        return false;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        std::cerr << "fcntl(F_SETFL) failed: "
                  << std::strerror(errno) << '\n';

        return false;
    }

    return true;
}


// ============================================================
// Modify events registered for a client
// ============================================================

bool modifyClientEvents(int epollFd,
                        const ClientConnection& client,
                        uint32_t events)
{
    epoll_event event{};

    event.events = events | EPOLLET;
    event.data.fd = client.fd;

    if (epoll_ctl(
            epollFd,
            EPOLL_CTL_MOD,
            client.fd,
            &event) == -1)
    {
        std::cerr << "epoll_ctl(MOD) failed for fd "
                  << client.fd
                  << ": "
                  << std::strerror(errno)
                  << '\n';

        return false;
    }

    return true;
}


// ============================================================
// Enable EPOLLOUT
// ============================================================

bool enableWrite(int epollFd,
                 const ClientConnection& client)
{
    return modifyClientEvents(
        epollFd,
        client,
        EPOLLIN | EPOLLOUT
    );
}


// ============================================================
// Disable EPOLLOUT
// ============================================================

bool disableWrite(int epollFd,
                  const ClientConnection& client)
{
    return modifyClientEvents(
        epollFd,
        client,
        EPOLLIN
    );
}


// ============================================================
// Handle writing
// ============================================================

bool handleWrite(int epollFd,
                 ClientConnection& client)
{
    while (client.outputOffset < client.outputBuffer.size())
    {
        const char* data =
            client.outputBuffer.data()
            + client.outputOffset;

        std::size_t remaining =
            client.outputBuffer.size()
            - client.outputOffset;

        ssize_t bytesSent =
            send(
                client.fd,
                data,
                remaining,
                0
            );

        // ----------------------------------------------------
        // send() successfully sent some bytes
        // ----------------------------------------------------

        if (bytesSent > 0)
        {
            client.outputOffset += bytesSent;

            std::cout
                << "fd=" << client.fd
                << " sent "
                << bytesSent
                << " bytes, "
                << "remaining="
                << client.outputBuffer.size()
                   - client.outputOffset
                << '\n';

            continue;
        }


        // ----------------------------------------------------
        // send() returned 0
        // ----------------------------------------------------

        if (bytesSent == 0)
        {
            std::cout
                << "send() returned 0 for fd="
                << client.fd
                << '\n';

            return false;
        }


        // ----------------------------------------------------
        // Socket cannot accept more data right now
        // ----------------------------------------------------

        if (errno == EAGAIN ||
            errno == EWOULDBLOCK)
        {
            std::cout
                << "fd=" << client.fd
                << " send would block. "
                << "Waiting for EPOLLOUT...\n";

            return true;
        }


        // ----------------------------------------------------
        // Real error
        // ----------------------------------------------------

        std::cerr
            << "send() failed for fd="
            << client.fd
            << ": "
            << std::strerror(errno)
            << '\n';

        return false;
    }


    // ========================================================
    // Everything was sent
    // ========================================================

    std::cout
        << "fd=" << client.fd
        << " output buffer completely sent\n";


    client.outputBuffer.clear();
    client.outputOffset = 0;


    // We don't need EPOLLOUT anymore
    disableWrite(epollFd, client);

    return true;
}


// ============================================================
// Handle reading
// ============================================================

bool handleRead(int epollFd,
                ClientConnection& client)
{
    while (true)
    {
        char buffer[BUFFER_SIZE];

        ssize_t bytesRead =
            recv(
                client.fd,
                buffer,
                sizeof(buffer),
                0
            );


        // ----------------------------------------------------
        // Data received
        // ----------------------------------------------------

        if (bytesRead > 0)
        {
            std::cout
                << "fd=" << client.fd
                << " received "
                << bytesRead
                << " bytes\n";

            std::cout
                << "Data: ";

            std::cout.write(buffer, bytesRead);

            std::cout << '\n';


            // =================================================
            // Add received data to output buffer
            // =================================================

            client.outputBuffer.append(
                buffer,
                bytesRead
            );


            std::cout
                << "outputBuffer size = "
                << client.outputBuffer.size()
                << '\n';


            // =================================================
            // We now have data to send
            // =================================================

            enableWrite(
                epollFd,
                client
            );


            continue;
        }


        // ----------------------------------------------------
        // Client closed connection
        // ----------------------------------------------------

        if (bytesRead == 0)
        {
            std::cout
                << "Client fd="
                << client.fd
                << " closed connection\n";

            return false;
        }


        // ----------------------------------------------------
        // No more data available right now
        // ----------------------------------------------------

        if (errno == EAGAIN ||
            errno == EWOULDBLOCK)
        {
            break;
        }


        // ----------------------------------------------------
        // Real error
        // ----------------------------------------------------

        std::cerr
            << "recv() failed for fd="
            << client.fd
            << ": "
            << std::strerror(errno)
            << '\n';

        return false;
    }

    return true;
}


// ============================================================
// Close client
// ============================================================

void closeClient(int epollFd,
                 ClientConnection& client)
{
    std::cout
        << "Closing client fd="
        << client.fd
        << '\n';

    epoll_ctl(
        epollFd,
        EPOLL_CTL_DEL,
        client.fd,
        nullptr
    );

    close(client.fd);
}


// ============================================================
// Main
// ============================================================

int main()
{
    // ========================================================
    // 1. Create listening socket
    // ========================================================

    int serverFd =
        socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (serverFd == -1)
    {
        std::cerr
            << "socket() failed: "
            << std::strerror(errno)
            << '\n';

        return 1;
    }


    // ========================================================
    // 2. Make listening socket non-blocking
    // ========================================================

    if (!setNonBlocking(serverFd))
    {
        close(serverFd);
        return 1;
    }


    // ========================================================
    // 3. Allow address reuse
    // ========================================================

    int reuse = 1;

    if (setsockopt(
            serverFd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuse,
            sizeof(reuse)) == -1)
    {
        std::cerr
            << "setsockopt() failed: "
            << std::strerror(errno)
            << '\n';

        close(serverFd);
        return 1;
    }


    // ========================================================
    // 4. Bind
    // ========================================================

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);


    if (bind(
            serverFd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == -1)
    {
        std::cerr
            << "bind() failed: "
            << std::strerror(errno)
            << '\n';

        close(serverFd);
        return 1;
    }


    // ========================================================
    // 5. Listen
    // ========================================================

    if (listen(serverFd, SOMAXCONN) == -1)
    {
        std::cerr
            << "listen() failed: "
            << std::strerror(errno)
            << '\n';

        close(serverFd);
        return 1;
    }


    std::cout
        << "Server listening on port "
        << PORT
        << '\n';


    // ========================================================
    // 6. Create epoll instance
    // ========================================================

    int epollFd =
        epoll_create1(0);

    if (epollFd == -1)
    {
        std::cerr
            << "epoll_create1() failed: "
            << std::strerror(errno)
            << '\n';

        close(serverFd);
        return 1;
    }


    // ========================================================
    // 7. Add server socket to epoll
    // ========================================================

    epoll_event serverEvent{};

    serverEvent.events =
        EPOLLIN |
        EPOLLET;

    serverEvent.data.fd =
        serverFd;


    if (epoll_ctl(
            epollFd,
            EPOLL_CTL_ADD,
            serverFd,
            &serverEvent) == -1)
    {
        std::cerr
            << "epoll_ctl(ADD serverFd) failed: "
            << std::strerror(errno)
            << '\n';

        close(epollFd);
        close(serverFd);

        return 1;
    }


    // ========================================================
    // Client storage
    // ========================================================

    // For learning purposes, we use a simple map.
    // fd -> ClientConnection
    //
    // In production, you may use unique_ptr/shared_ptr or
    // another connection manager.

    std::unordered_map<int, ClientConnection> clients;


    // ========================================================
    // epoll event array
    // ========================================================

    epoll_event events[MAX_EVENTS];


    // ========================================================
    // Event loop
    // ========================================================

    while (true)
    {
        int eventCount =
            epoll_wait(
                epollFd,
                events,
                MAX_EVENTS,
                -1
            );


        if (eventCount == -1)
        {
            if (errno == EINTR)
                continue;

            std::cerr
                << "epoll_wait() failed: "
                << std::strerror(errno)
                << '\n';

            break;
        }


        // ====================================================
        // Process ready events
        // ====================================================

        for (int i = 0;
             i < eventCount;
             ++i)
        {
            int fd =
                events[i].data.fd;

            uint32_t eventMask =
                events[i].events;


            // =================================================
            // Listening socket
            // =================================================

            if (fd == serverFd)
            {
                while (true)
                {
                    int clientFd =
                        accept(
                            serverFd,
                            nullptr,
                            nullptr
                        );


                    if (clientFd == -1)
                    {
                        if (errno == EAGAIN ||
                            errno == EWOULDBLOCK)
                        {
                            break;
                        }

                        std::cerr
                            << "accept() failed: "
                            << std::strerror(errno)
                            << '\n';

                        break;
                    }


                    // -----------------------------------------
                    // Make client non-blocking
                    // -----------------------------------------

                    if (!setNonBlocking(clientFd))
                    {
                        close(clientFd);
                        continue;
                    }


                    // -----------------------------------------
                    // Add client to epoll
                    // -----------------------------------------

                    epoll_event clientEvent{};

                    clientEvent.events =
                        EPOLLIN |
                        EPOLLET;

                    clientEvent.data.fd =
                        clientFd;


                    if (epoll_ctl(
                            epollFd,
                            EPOLL_CTL_ADD,
                            clientFd,
                            &clientEvent) == -1)
                    {
                        std::cerr
                            << "epoll_ctl(ADD client) failed: "
                            << std::strerror(errno)
                            << '\n';

                        close(clientFd);
                        continue;
                    }


                    // -----------------------------------------
                    // Create connection object
                    // -----------------------------------------

                    ClientConnection client;

                    client.fd = clientFd;

                    clients.emplace(
                        clientFd,
                        std::move(client)
                    );


                    std::cout
                        << "New client connected. fd="
                        << clientFd
                        << '\n';
                }

                continue;
            }


            // =================================================
            // Find client
            // =================================================

            auto it =
                clients.find(fd);

            if (it == clients.end())
            {
                continue;
            }


            ClientConnection& client =
                it->second;


            // =================================================
            // Error / hangup
            // =================================================

            if (eventMask &
                (EPOLLERR |
                 EPOLLHUP |
                 EPOLLRDHUP))
            {
                closeClient(
                    epollFd,
                    client
                );

                clients.erase(it);

                continue;
            }


            // =================================================
            // Readable
            // =================================================

            if (eventMask & EPOLLIN)
            {
                if (!handleRead(
                        epollFd,
                        client))
                {
                    closeClient(
                        epollFd,
                        client
                    );

                    clients.erase(it);

                    continue;
                }
            }


            // =================================================
            // Writable
            // =================================================

            if (eventMask & EPOLLOUT)
            {
                if (!handleWrite(
                        epollFd,
                        client))
                {
                    closeClient(
                        epollFd,
                        client
                    );

                    clients.erase(it);

                    continue;
                }
            }
        }
    }


    // ========================================================
    // Cleanup
    // ========================================================

    for (auto& [fd, client] : clients)
    {
        close(client.fd);
    }

    close(epollFd);
    close(serverFd);

    return 0;
}
