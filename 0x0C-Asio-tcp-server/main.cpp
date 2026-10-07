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

    acceptor.accept(socket); // program will block here until a client connects
    // this is a synchronous acceptor, it will block until a client connects

    std::cout << "Client connected!\n";

    return 0;
}
