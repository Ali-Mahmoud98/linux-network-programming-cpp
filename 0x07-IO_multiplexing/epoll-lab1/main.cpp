#include <sys/epoll.h>
#include <unistd.h>

#include <iostream>

int main()
{
    int epfd = epoll_create1(0);

    if (epfd == -1)
    {
        perror("epoll_create1");
        return 1;
    }

    std::cout << "epoll fd = "
              << epfd
              << '\n';

    close(epfd);

    return 0;
}
