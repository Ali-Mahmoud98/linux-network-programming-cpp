#include <iostream>
#include <sys/select.h>
#include <unistd.h>

int main()
{
    fd_set readfds;

    FD_ZERO(&readfds);

    FD_SET(STDIN_FILENO, &readfds);

    std::cout << "Waiting for input...\n";

    int result = select(
        STDIN_FILENO + 1,
        &readfds,
        nullptr,
        nullptr,
        nullptr
    );

    if (result == -1)
    {
        perror("select");
        return 1;
    }

    if (FD_ISSET(STDIN_FILENO, &readfds))
    {
        std::cout << "stdin is ready!\n";
    }

    return 0;
}
