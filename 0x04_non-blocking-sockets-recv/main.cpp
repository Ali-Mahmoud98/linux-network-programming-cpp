#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include <cerrno>
#include <cstring>

bool setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL);

    if (flags == -1)
    {
        return false;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        return false;
    }

    return true;
}

int main()
{
    int server_fd = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if (server_fd == -1)
    {
        perror("socket");
        return 1;
    }

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(12345);

    if (bind(
            server_fd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == -1)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, SOMAXCONN) == -1)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    if (!setNonBlocking(server_fd))
    {
        perror("setNonBlocking");
        close(server_fd);
        return 1;
    }

    std::cout << "Server listening...\n";

    while (true)
    {
        int client_fd = accept(
            server_fd,
            nullptr,
            nullptr
        );

        if (client_fd == -1)
        {
            if (errno == EAGAIN ||
                errno == EWOULDBLOCK)
            {
                std::cout << "No client yet...\n";
                sleep(1);
                continue;
            }

            perror("accept");
            break;
        }

        std::cout << "Client connected!\n";

        if (!setNonBlocking(client_fd))
        {
            perror("setNonBlocking client");
            close(client_fd);
            continue;
        }

        while (true)
        {
            char buffer[1024];

            ssize_t n = recv(
                client_fd,
                buffer,
                sizeof(buffer),
                0
            );

            if (n > 0)
            {
                std::cout << "Received "
                          << n
                          << " bytes: ";

                std::cout.write(buffer, n);
                std::cout << '\n';
            }
            else if (n == 0)
            {
                std::cout
                    << "Client closed connection\n";

                break;
            }
            else
            {
                if (errno == EAGAIN ||
                    errno == EWOULDBLOCK)
                {
                    std::cout
                        << "No data available yet\n";

                    sleep(1);
                    continue;
                }

                std::cerr
                    << "recv failed: "
                    << strerror(errno)
                    << '\n';

                break;
            }
        }

        close(client_fd);
    }

    close(server_fd);
}
