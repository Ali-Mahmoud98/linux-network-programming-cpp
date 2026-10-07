#include "EpollServer.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
constexpr int MAX_EVENTS = 64;
constexpr int BUFFER_SIZE = 1024;
}


// ============================================================
// Constructor
// ============================================================

EpollServer::EpollServer(int port)
    : m_port(port)
{
}


// ============================================================
// Destructor
// ============================================================

EpollServer::~EpollServer()
{
    if (m_epollFd != -1)
    {
        close(m_epollFd);
    }

    if (m_serverFd != -1)
    {
        close(m_serverFd);
    }
}


// ============================================================
// Set non-blocking
// ============================================================

bool EpollServer::setNonBlocking(int fd)
{
    int flags =
        fcntl(
            fd,
            F_GETFL,
            0
        );

    if (flags == -1)
    {
        return false;
    }

    return fcntl(
        fd,
        F_SETFL,
        flags | O_NONBLOCK
    ) != -1;
}


// ============================================================
// Setup
// ============================================================

bool EpollServer::setup()
{
    // --------------------------------------------------------
    // socket()
    // --------------------------------------------------------

    m_serverFd =
        ::socket(
            AF_INET,
            SOCK_STREAM,
            0
        );

    if (m_serverFd == -1)
    {
        std::cerr
            << "socket() failed: "
            << std::strerror(errno)
            << '\n';

        return false;
    }


    // --------------------------------------------------------
    // non-blocking
    // --------------------------------------------------------

    if (!setNonBlocking(m_serverFd))
    {
        return false;
    }


    // --------------------------------------------------------
    // SO_REUSEADDR
    // --------------------------------------------------------

    int reuse = 1;

    if (setsockopt(
            m_serverFd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &reuse,
            sizeof(reuse)) == -1)
    {
        return false;
    }


    // --------------------------------------------------------
    // bind()
    // --------------------------------------------------------

    sockaddr_in address{};

    address.sin_family =
        AF_INET;

    address.sin_addr.s_addr =
        INADDR_ANY;

    address.sin_port =
        htons(m_port);


    if (bind(
            m_serverFd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == -1)
    {
        return false;
    }


    // --------------------------------------------------------
    // listen()
    // --------------------------------------------------------

    if (listen(
            m_serverFd,
            SOMAXCONN) == -1)
    {
        return false;
    }


    // --------------------------------------------------------
    // epoll_create1()
    // --------------------------------------------------------

    m_epollFd =
        epoll_create1(0);

    if (m_epollFd == -1)
    {
        return false;
    }


    // --------------------------------------------------------
    // Add listening socket
    // --------------------------------------------------------

    epoll_event event{};

    event.events =
        EPOLLIN |
        EPOLLET;

    event.data.fd =
        m_serverFd;


    if (epoll_ctl(
            m_epollFd,
            EPOLL_CTL_ADD,
            m_serverFd,
            &event) == -1)
    {
        return false;
    }


    std::cout
        << "Server listening on port "
        << m_port
        << '\n';

    return true;
}


// ============================================================
// Accept clients
// ============================================================

void EpollServer::acceptClients()
{
    while (true)
    {
        int clientFd =
            accept(
                m_serverFd,
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


        if (!setNonBlocking(clientFd))
        {
            close(clientFd);
            continue;
        }


        auto client =
            std::make_unique<ClientConnection>(
                clientFd
            );


        epoll_event event{};

        event.events =
            EPOLLIN |
            EPOLLET;

        event.data.fd =
            clientFd;


        if (epoll_ctl(
                m_epollFd,
                EPOLL_CTL_ADD,
                clientFd,
                &event) == -1)
        {
            close(clientFd);
            continue;
        }


        m_clients.emplace(
            clientFd,
            std::move(client)
        );


        std::cout
            << "Client connected: fd="
            << clientFd
            << '\n';
    }
}


// ============================================================
// Read client
// ============================================================

void EpollServer::handleClientRead(
    ClientConnection& client)
{
    char buffer[BUFFER_SIZE];

    while (true)
    {
        ssize_t bytesRead =
            client.socket().receiveData(
                buffer,
                sizeof(buffer)
            );


        if (bytesRead > 0)
        {
            std::cout
                << "Received "
                << bytesRead
                << " bytes from fd="
                << client.fd()
                << '\n';

            std::cout << "Data: ";

            std::cout.write(
                buffer,
                bytesRead
            );

            std::cout << '\n';


            // ----------------------------------------------
            // Echo
            // ----------------------------------------------

            client.appendOutput(
                buffer,
                bytesRead
            );

            updateClientEvents(client);

            continue;
        }


        if (bytesRead == 0)
        {
            closeClient(client.fd());
            return;
        }


        if (errno == EAGAIN ||
            errno == EWOULDBLOCK)
        {
            break;
        }


        std::cerr
            << "recv() failed for fd="
            << client.fd()
            << '\n';

        closeClient(client.fd());
        return;
    }
}


// ============================================================
// Write client
// ============================================================

void EpollServer::handleClientWrite(
    ClientConnection& client)
{
    if (!client.writeAvailable())
    {
        closeClient(client.fd());
        return;
    }


    updateClientEvents(client);
}


// ============================================================
// Update EPOLL events
// ============================================================

void EpollServer::updateClientEvents(
    ClientConnection& client)
{
    epoll_event event{};

    event.events =
        EPOLLIN |
        EPOLLET;


    if (client.hasPendingOutput())
    {
        event.events |= EPOLLOUT;
    }


    event.data.fd =
        client.fd();


    if (epoll_ctl(
            m_epollFd,
            EPOLL_CTL_MOD,
            client.fd(),
            &event) == -1)
    {
        std::cerr
            << "epoll_ctl(MOD) failed: "
            << std::strerror(errno)
            << '\n';
    }
}


// ============================================================
// Close client
// ============================================================

void EpollServer::closeClient(int fd)
{
    auto it =
        m_clients.find(fd);

    if (it == m_clients.end())
    {
        return;
    }


    std::cout
        << "Closing client fd="
        << fd
        << '\n';


    epoll_ctl(
        m_epollFd,
        EPOLL_CTL_DEL,
        fd,
        nullptr
    );


    m_clients.erase(it);
}


// ============================================================
// Event loop
// ============================================================

void EpollServer::start()
{
    if (!setup())
    {
        return;
    }


    epoll_event events[MAX_EVENTS];


    while (true)
    {
        int count =
            epoll_wait(
                m_epollFd,
                events,
                MAX_EVENTS,
                -1
            );


        if (count == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }

            std::cerr
                << "epoll_wait() failed: "
                << std::strerror(errno)
                << '\n';

            break;
        }


        for (int i = 0;
             i < count;
             ++i)
        {
            int fd =
                events[i].data.fd;

            uint32_t eventMask =
                events[i].events;


            // =================================================
            // Listening socket
            // =================================================

            if (fd == m_serverFd)
            {
                acceptClients();
                continue;
            }


            // =================================================
            // Find client
            // =================================================

            auto it =
                m_clients.find(fd);

            if (it == m_clients.end())
            {
                continue;
            }


            ClientConnection& client =
                *it->second;


            // =================================================
            // Error / disconnect
            // =================================================

            if (eventMask &
                (EPOLLERR |
                 EPOLLHUP |
                 EPOLLRDHUP))
            {
                closeClient(fd);
                continue;
            }


            // =================================================
            // Read
            // =================================================

            if (eventMask & EPOLLIN)
            {
                handleClientRead(client);
            }


            // =================================================
            // Client might have been removed
            // =================================================

            if (m_clients.find(fd) ==
                m_clients.end())
            {
                continue;
            }


            // =================================================
            // Write
            // =================================================

            if (eventMask & EPOLLOUT)
            {
                auto clientIt =
                    m_clients.find(fd);

                if (clientIt ==
                    m_clients.end())
                {
                    continue;
                }

                handleClientWrite(
                    *clientIt->second
                );
            }
        }
    }
}
