#include <iostream>
#include "utils.hpp"
#include "Controller.hpp"

int main(int argc, const char* argv[]) {
    std::cout << std::boolalpha;
    std::cout.sync_with_stdio(false);
    
    std::cout << "Hello, World! DDS" << std::endl;
    
    core::Controller c;
    std::cout << c.foo() << endl;
    std::cout << c.inline_foo() << endl;
    
    return EXIT_SUCCESS;
}