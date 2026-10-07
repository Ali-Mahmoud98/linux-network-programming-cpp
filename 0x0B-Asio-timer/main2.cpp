#include <asio.hpp>
#include <iostream>
#include <chrono>

int main()
{
    asio::io_context io;

    asio::steady_timer timer(
        io,
        std::chrono::seconds(5)
    );

    asio::steady_timer timer2(
        io,
        std::chrono::seconds(1)
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

    timer2.async_wait(
        [](const asio::error_code& error)
        {
            if (!error)
            {
                std::cout << "Timer2 expired!\n";
            }
        }
    );

    std::cout << "A\n";
    std::cout << "B\n";
    std::cout << "C\n";

    io.run();

    std::cout << "D\n";

    return 0;
}
