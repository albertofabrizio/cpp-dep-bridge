#include <iostream>
#include "depbridge/version.hpp"

int main(int, char**) {
    std::cout << "cpp-dep-bridge " << depbridge::version << "\n";
    return 0;
}
