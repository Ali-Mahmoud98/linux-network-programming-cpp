#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(12345);

    bind(
        server_fd,
        reinterpret_cast<sockaddr*>(&address),
        sizeof(address)
    );

    listen(server_fd, SOMAXCONN);

    int flags = fcntl(server_fd, F_GETFL, 0);

    fcntl(
        server_fd,
        F_SETFL,
        flags | O_NONBLOCK
    );

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
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::cout << "No client yet\n";
                sleep(1);
                continue;
            }

            std::cerr << "accept failed: "
                      << strerror(errno)
                      << '\n';

            break;
        }

        std::cout << "Client connected!\n";

        close(client_fd);
    }

    close(server_fd);
}
