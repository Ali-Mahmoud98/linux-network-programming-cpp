#include <asio.hpp>
#include <iostream>


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

    std::cout << "Server listening on port 8080...\n";

    asio::ip::tcp::socket socket(io);

    acceptor.async_accept(
        socket,
        [](const asio::error_code& ec){
            if (!ec) {
                std::cout << "Client connected!\n";
            } else {
                std::cout << "Accept failed: " << ec.message() << "\n";
            }
        }
    );

    std::cout << "Before io.run()\n";

    io.run();

    std::cout << "After io.run()\n";

    return 0;
}
