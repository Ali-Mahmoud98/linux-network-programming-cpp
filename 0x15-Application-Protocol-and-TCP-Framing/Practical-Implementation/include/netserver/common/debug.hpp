#pragma once

#include <iostream>


#ifdef DEBUG_TRACE

#define DEBUG_PRINT(...)                                              \
do {                                                              \
    std::cout << "[DEBUG] "                                       \
                << __FILE__ << ":" << __LINE__ << " "               \
                << __VA_ARGS__ << std::endl;                        \
} while (0)

#define DEBUG_PRINT_ERR(...) \
do { \
    std::cerr << "[DEBUG] " << __FILE__ << ":" << __LINE__ << " " << __VA_ARGS__ << std::endl; \
} while (0)

#else

// Compiles to nothing; arguments are NOT evaluated (zero cost).
#define DEBUG_PRINT(...) ((void)0)
#define DEBUG_PRINT_ERR(...) ((void)0)
#endif
