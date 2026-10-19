#include <asio.hpp>
#include <iostream>
#include <string>

int main()
{
    asio::io_context io_context;

    asio::ip::tcp::acceptor acceptor(io_context,
        asio::ip::tcp::endpoint(asio::ip::tcp::v4(),
        12345)
    );

    asio::ip::tcp::socket socket(io_context);

    std::string message = "Hello, client!";

    acceptor.async_accept(socket,
        [&socket, &message](const asio::error_code& error)
        {
            if (!error)
            {
                // Connection accepted successfully.
                std::cout << "Connection accepted successfully." << std::endl;
                asio::async_write(socket, asio::buffer(message),
                    [](const asio::error_code& error, std::size_t /*bytes_transferred*/)
                    {
                        if (!error)
                        {
                            std::cout << "Message sent to client successfully." << std::endl;
                        }
                    }
                );
                std::cout << "Finished handling client connection." << std::endl;
                std::cout << "End async_accept callback." << std::endl;
            }
        }
    );

    std::cout << "Waiting for incoming connections..." << std::endl;

    io_context.run();
}
