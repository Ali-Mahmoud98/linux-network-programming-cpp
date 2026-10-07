#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

int main()
{
    int fd = socket(
        AF_INET,
        SOCK_STREAM,
        0);

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_port = htons(12345);

    inet_pton(
        AF_INET,
        "127.0.0.1",
        &address.sin_addr);

    if (connect(
            fd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == -1)
    {
        perror("connect");
        return 1;
    }

    std::cout
        << "Connected.\n";

    std::cout
        << "I will NOT read from socket.\n";

    while (true)
    {
        sleep(1);
    }

    close(fd);
}
