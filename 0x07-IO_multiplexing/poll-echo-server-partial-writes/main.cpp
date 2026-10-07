#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

constexpr int PORT = 12345;
constexpr int MAX_CLIENTS = 10;

struct ClientConnection
{
    int fd = -1;

    std::string outputBuffer;

    std::size_t outputOffset = 0;
};

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

void closeClient(ClientConnection& client)
{
    if (client.fd != -1)
    {
        std::cout
            << "Closing client fd="
            << client.fd
            << '\n';

        close(client.fd);

        client.fd = -1;
    }

    client.outputBuffer.clear();
    client.outputOffset = 0;
}

bool hasPendingData(const ClientConnection& client)
{
    return client.outputOffset <
           client.outputBuffer.size();
}

void enableWriteEvent(
    pollfd& pfd,
    const ClientConnection& client)
{
    if (hasPendingData(client))
    {
        pfd.events |= POLLOUT;
    }
    else
    {
        pfd.events &= ~POLLOUT;
    }
}

bool handleRead(
    ClientConnection& client)
{
    char buffer[4096];

    while (true)
    {
        ssize_t bytes = recv(
            client.fd,
            buffer,
            sizeof(buffer),
            0);

        if (bytes > 0)
        {
            std::cout
                << "Received "
                << bytes
                << " bytes from fd="
                << client.fd
                << '\n';

            /*
             * Echo the received data.
             */
            client.outputBuffer.append(
                buffer,
                bytes);

            continue;
        }

        if (bytes == 0)
        {
            /*
             * Client closed connection.
             */
            return false;
        }

        /*
         * recv() failed.
         */

        if (errno == EAGAIN ||
            errno == EWOULDBLOCK)
        {
            /*
             * No more data currently available.
             */
            return true;
        }

        perror("recv");

        return false;
    }
}

bool handleWrite(
    ClientConnection& client)
{
    while (hasPendingData(client))
    {
        const char* data =
            client.outputBuffer.data()
            + client.outputOffset;

        std::size_t remaining =
            client.outputBuffer.size()
            - client.outputOffset;

        ssize_t bytes = send(
            client.fd,
            data,
            remaining,
            0);

        if (bytes > 0)
        {
            client.outputOffset += bytes;

            std::cout
                << "Sent "
                << bytes
                << " bytes to fd="
                << client.fd
                << '\n';

            continue;
        }

        if (bytes == -1)
        {
            if (errno == EAGAIN ||
                errno == EWOULDBLOCK)
            {
                /*
                 * Send buffer is currently full.
                 *
                 * Keep the remaining data and
                 * wait for POLLOUT.
                 */
                return true;
            }

            perror("send");

            return false;
        }
    }

    /*
     * Everything has been sent.
     */
    client.outputBuffer.clear();
    client.outputOffset = 0;

    return true;
}

int main()
{
    // --------------------------------------------------
    // 1. Create server socket
    // --------------------------------------------------

    int serverFd = socket(
        AF_INET,
        SOCK_STREAM,
        0);

    if (serverFd == -1)
    {
        perror("socket");
        return 1;
    }

    // --------------------------------------------------
    // 2. SO_REUSEADDR
    // --------------------------------------------------

    int option = 1;

    if (setsockopt(
            serverFd,
            SOL_SOCKET,
            SO_REUSEADDR,
            &option,
            sizeof(option)) == -1)
    {
        perror("setsockopt");

        close(serverFd);

        return 1;
    }

    // --------------------------------------------------
    // 3. Bind
    // --------------------------------------------------

    sockaddr_in serverAddress{};

    serverAddress.sin_family =
        AF_INET;

    serverAddress.sin_addr.s_addr =
        INADDR_ANY;

    serverAddress.sin_port =
        htons(PORT);

    if (bind(
            serverFd,
            reinterpret_cast<sockaddr*>(
                &serverAddress),
            sizeof(serverAddress)) == -1)
    {
        perror("bind");

        close(serverFd);

        return 1;
    }

    // --------------------------------------------------
    // 4. Listen
    // --------------------------------------------------

    if (listen(serverFd, 10) == -1)
    {
        perror("listen");

        close(serverFd);

        return 1;
    }

    // --------------------------------------------------
    // 5. Non-blocking server socket
    // --------------------------------------------------

    if (setNonBlocking(serverFd) == -1)
    {
        perror("fcntl");

        close(serverFd);

        return 1;
    }

    std::cout
        << "Server listening on port "
        << PORT
        << '\n';

    // --------------------------------------------------
    // 6. poll structures
    // --------------------------------------------------

    pollfd pollFds[MAX_CLIENTS + 1]{};

    ClientConnection clients[MAX_CLIENTS];

    /*
     * Index 0 = listening socket
     */

    pollFds[0].fd = serverFd;
    pollFds[0].events = POLLIN;

    /*
     * Empty client slots
     */

    for (int i = 1;
         i <= MAX_CLIENTS;
         ++i)
    {
        pollFds[i].fd = -1;
    }

    // --------------------------------------------------
    // 7. Event loop
    // --------------------------------------------------

    while (true)
    {
        /*
         * Make sure POLLOUT reflects
         * whether each client has
         * pending output.
         */

        for (int i = 1;
             i <= MAX_CLIENTS;
             ++i)
        {
            if (clients[i - 1].fd == -1)
                continue;

            enableWriteEvent(
                pollFds[i],
                clients[i - 1]);
        }

        // --------------------------------------------------
        // Wait for events
        // --------------------------------------------------

        int ready = poll(
            pollFds,
            MAX_CLIENTS + 1,
            -1);

        if (ready == -1)
        {
            if (errno == EINTR)
                continue;

            perror("poll");

            break;
        }

        // --------------------------------------------------
        // 8. New connection
        // --------------------------------------------------

        if (pollFds[0].revents & POLLIN)
        {
            while (true)
            {
                sockaddr_in clientAddress{};
                socklen_t clientLength =
                    sizeof(clientAddress);

                int clientFd = accept(
                    serverFd,
                    reinterpret_cast<sockaddr*>(
                        &clientAddress),
                    &clientLength);

                if (clientFd == -1)
                {
                    if (errno == EAGAIN ||
                        errno == EWOULDBLOCK)
                    {
                        break;
                    }

                    perror("accept");

                    break;
                }

                if (setNonBlocking(clientFd) == -1)
                {
                    perror("fcntl");

                    close(clientFd);

                    continue;
                }

                bool added = false;

                for (int i = 0;
                     i < MAX_CLIENTS;
                     ++i)
                {
                    if (clients[i].fd == -1)
                    {
                        clients[i].fd = clientFd;

                        pollFds[i + 1].fd =
                            clientFd;

                        pollFds[i + 1].events =
                            POLLIN;

                        pollFds[i + 1].revents = 0;

                        added = true;

                        std::cout
                            << "New client fd="
                            << clientFd
                            << '\n';

                        break;
                    }
                }

                if (!added)
                {
                    std::cout
                        << "Maximum clients reached\n";

                    close(clientFd);
                }
            }
        }

        // --------------------------------------------------
        // 9. Process clients
        // --------------------------------------------------

        for (int i = 0;
             i < MAX_CLIENTS;
             ++i)
        {
            ClientConnection& client =
                clients[i];

            pollfd& pfd =
                pollFds[i + 1];

            if (client.fd == -1)
                continue;

            // --------------------------------------------
            // Error / hangup
            // --------------------------------------------

            if (pfd.revents &
                (POLLERR | POLLHUP | POLLNVAL))
            {
                closeClient(client);

                pfd.fd = -1;
                pfd.events = 0;

                continue;
            }

            // --------------------------------------------
            // Read
            // --------------------------------------------

            if (pfd.revents & POLLIN)
            {
                if (!handleRead(client))
                {
                    closeClient(client);

                    pfd.fd = -1;
                    pfd.events = 0;

                    continue;
                }
            }

            // --------------------------------------------
            // Write
            // --------------------------------------------

            if (pfd.revents & POLLOUT)
            {
                if (!handleWrite(client))
                {
                    closeClient(client);

                    pfd.fd = -1;
                    pfd.events = 0;

                    continue;
                }
            }

            // --------------------------------------------
            // Update POLLOUT
            // --------------------------------------------

            enableWriteEvent(
                pfd,
                client);
        }
    }

    // --------------------------------------------------
    // Cleanup
    // --------------------------------------------------

    for (auto& client : clients)
    {
        closeClient(client);
    }

    close(serverFd);

    return 0;
}
