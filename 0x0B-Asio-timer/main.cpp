#include <asio.hpp>
#include <iostream>
#include <chrono>

int main()
{
    asio::io_context io;

    asio::steady_timer timer(
        io,
        std::chrono::seconds(2)
    );

    timer.async_wait(
        [](const asio::error_code& error)
        {
            if (!error)
            {
                std::cout << "Timer expired!\n";
            }
        }
    );

    std::cout << "Before io.run()\n";

    io.run();

    std::cout << "After io.run()\n";

    return 0;
}
