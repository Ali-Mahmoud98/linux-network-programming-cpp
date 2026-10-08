#include <asio.hpp>
#include <iostream>
#include <memory> // for std::make_shared
#include <array> // for std::array

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
    asio::ip::tcp::socket socket(io); // create a socket

    acceptor.async_accept(
        socket,
        [&](const asio::error_code& ec){
            if (ec) {
                std::cerr << "Accept error: " << ec.message() << "\n";
                return;
            }

            std::cout << "Client connected!\n";

            auto buffer = std::make_shared<std::array<char, 1024>>(); // create a buffer for reading and writing data
            // we path the buffer to async_read_some and async_write_some callbacks to **keep it alive**
            //we path it by value, because it is a shared pointer -> reference count is increased

            socket.async_read_some(
                asio::buffer(*buffer),
                [&, buffer](const asio::error_code& ec,
                        std::size_t bytes_read)
                {
                    if (ec) {
                        std::cerr << "Read error: " << ec.message() << "\n";
                        return;
                    }

                    std::cout << "Received " << bytes_read << " bytes from client.\n";

                    // Echo the received data back to the client
                    socket.async_write_some(
                        asio::buffer(buffer->data(), bytes_read),
                        [&, buffer](const asio::error_code& ec,
                                std::size_t bytes_written){
                            if (ec) {
                                std::cerr << "Write error: " << ec.message() << "\n";
                                return;
                            }
                            std::cout << "Sent " << bytes_written << " bytes to client.\n";
                        }
                    );
                }
            );
            // Test with the following line commented out and test with the following line uncommented
            // std::cout << "Handler finished - Accept callback completed\n"; // this will be printed
            // before read or write callbacks, because async operations are non-blocking
        }
    );

    std::cout << "Server Started\n";

    io.run();

    std::cout << "Server Stopped\n";


    return 0;
}
