#include <asio.hpp>
#include "server/Server.hpp"

int main()
{
    asio::io_context io;

    Server server(io, 8080);

    server.start();

    io.run();
}