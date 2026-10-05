#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>

int main()
{
    int flags = fcntl(STDIN_FILENO, F_GETFL);

    if (flags == -1)
    {
        std::cerr << "F_GETFL failed: "
                  << strerror(errno)
                  << '\n';

        return 1;
    }

    flags |= O_NONBLOCK;

    if (fcntl(STDIN_FILENO, F_SETFL, flags) == -1)
    {
        std::cerr << "F_SETFL failed: "
                  << strerror(errno)
                  << '\n';

        return 1;
    }

    char buffer[100];

    while (true)
    {
        ssize_t n = read(
            STDIN_FILENO,
            buffer,
            sizeof(buffer)
        );

        if (n == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                std::cout << "No input yet\n";
                sleep(1);
                continue;
            }

            std::cerr << "read failed: "
                      << strerror(errno)
                      << '\n';

            break;
        }

        if (n > 0)
        {
            std::cout << "Received: ";

            std::cout.write(buffer, n);

            std::cout << '\n';
        }
    }
}
