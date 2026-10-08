#include <asio.hpp>
#include <iostream>

#include "connection/Connection.hpp"

int main()
{
    asio::io_context io;

    asio::ip::tcp::acceptor acceptor(
        io,
        asio::ip::tcp::endpoint(
            asio::ip::tcp::v4(),
            8080
        )
    );

    std::cout << "Server started on port 8080\n";

    auto connection =
        std::make_shared<Connection>(io);

    acceptor.async_accept(
        connection->socket(),
        [connection](const asio::error_code& error)
        {
            if (error)
            {
                std::cout << "Accept error: "
                          << error.message()
                          << '\n';
                return;
            }

            std::cout << "Client connected!\n";

            connection->start();
        }
    );

    io.run();

    return 0;
}
