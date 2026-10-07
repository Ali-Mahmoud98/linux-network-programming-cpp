#include "EpollServer.hpp"

int main()
{
    EpollServer server(12345);

    server.start();

    return 0;
}
