#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <vector>

constexpr std::size_t EXPECTED_SIZE = 10 * 1024 * 1024;
constexpr std::size_t READ_CHUNK = 64 * 1024;
constexpr int START_DELAY_SECONDS = 2;
constexpr useconds_t READ_DELAY_US = 10000;

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in address{};

    address.sin_family = AF_INET;
    address.sin_port = htons(12345);

    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    if (connect(
            fd,
            reinterpret_cast<sockaddr*>(&address),
            sizeof(address)) == -1)
    {
        perror("connect");
        return 1;
    }

    std::cout << "Connected. Not reading for "
              << START_DELAY_SECONDS
              << " seconds, so the server's buffers fill up.\n";

    sleep(START_DELAY_SECONDS);

    std::cout << "Reading slowly now.\n";

    std::vector<char> buffer(READ_CHUNK);

    std::size_t total = 0;
    bool corrupted = false;
    std::size_t badPosition = 0;

    // while (true) {} // try this with and look at the server's behavior when the client doesn't read

    while (total < EXPECTED_SIZE)
    {
        ssize_t n = recv(fd, buffer.data(), buffer.size(), 0);

        if (n == 0)
            break;

        if (n == -1)
        {
            perror("recv");
            break;
        }

        // Byte at position p must be 'A' + (p % 26).
        for (ssize_t i = 0; i < n && !corrupted; ++i)
        {
            std::size_t position = total + i;

            if (buffer[i] != static_cast<char>('A' + position % 26))
            {
                corrupted = true;
                badPosition = position;
            }
        }

        total += n;

        usleep(READ_DELAY_US);
    }

    std::cout << "Received " << total << " bytes\n";

    if (corrupted)
        std::cout << "DATA WRONG at byte " << badPosition << '\n';
    else if (total == EXPECTED_SIZE)
        std::cout << "Data OK\n";
    else
        std::cout << "Data incomplete\n";

    close(fd);
}
