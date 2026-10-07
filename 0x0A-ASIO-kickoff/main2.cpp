#include <asio.hpp>
#include <iostream>

int main()
{
    asio::io_context io;

    std::cout << "Before run()\n";

    io.run();

    std::cout << "After run()\n";

    return 0;
}
