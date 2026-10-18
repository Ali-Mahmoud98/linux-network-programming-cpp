#include <asio.hpp>
#include "Server.hpp"

#include "debug.hpp"


int main()
{
    try
    {
        asio::io_context io;

        Server server(io, 8080);
        server.start();

        // Signal handler: Ctrl+C
        asio::signal_set signals(io, SIGINT, SIGTERM); // Handle Ctrl+C and termination signals
        signals.async_wait(
            [&server](const asio::error_code& ec, int signal_number)
            {
                if (!ec)
                {
                    DEBUG_PRINT("Signal " << signal_number << " received. Initiating graceful shutdown...");
                    server.stop();
                }
            }
        );

        DEBUG_PRINT("Server running on port 8080. Press Ctrl+C to stop.");
        io.run();
        DEBUG_PRINT("Event loop exited cleanly.");
    }
    catch (const std::exception& e)
    {
        DEBUG_PRINT_ERR("Fatal exception: " << e.what());
        return 1;
    }

    return 0;
}
