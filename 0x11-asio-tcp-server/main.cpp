#include <asio.hpp>
#include "server/Server.hpp"

#include "debug/debug.h"

int main()
{
    asio::io_context io;

    Server server(io, 8080);
    DEBUG_PRINT("Main: Server starting");
    server.start();

    DEBUG_PRINT("Main: IO context running");
    io.run();
}