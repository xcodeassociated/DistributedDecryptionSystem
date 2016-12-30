#include <iostream>

#include "Controller.hpp"

constexpr auto endl = '\n';

int main(int argc, const char* argv[]) {
    std::cout << std::boolalpha;
    
    std::cout << "Hello, World! Slave node" << std::endl;
    
    core::Controller c;
    std::cout << c.foo() << endl;
    std::cout << c.inline_foo() << endl;
    
    return 0;
}