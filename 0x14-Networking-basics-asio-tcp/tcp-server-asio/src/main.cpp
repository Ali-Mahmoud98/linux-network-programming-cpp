#include <asio.hpp>
#include <exception>
#include <memory>

#include "netserver/app/EchoHandler.hpp"
#include "netserver/common/debug.hpp"
#include "netserver/transport/tcp/TcpServer.hpp"

int main()
{
    try
    {
        asio::io_context io;

        // 1. Create the application logic (Application Service)
        netserver::EchoHandler echo_handler;

        // 2. Create the transport adapter (TCP Transport Adapter) and bind it to the application
        std::unique_ptr<netserver::IMessageEndpoint> server =
            std::make_unique<netserver::TcpServer>(io, 8080); // Base class pointer points to derived object

        server->start([&echo_handler](const netserver::InboundMessage& msg) {
            echo_handler.on_message(msg);
        });

        // 3. Handling clean close signals (Ctrl+C)
        asio::signal_set signals(io, SIGINT, SIGTERM);
        signals.async_wait(
            [&server](const asio::error_code& ec, int signal_number)
            {
                if (!ec) {
                    DEBUG_PRINT("Signal " << signal_number << " received. Initiating graceful shutdown...");
                    server->stop();
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
