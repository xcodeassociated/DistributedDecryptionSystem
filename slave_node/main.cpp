#include <iostream>

#include "Controller.hpp"

int main(int argc, const char* argv[]) {
    std::cout << "Hello, World! Slave node" << std::endl;
    Controller c;
    c.foo();
    c.inline_foo();
    
    return 0;
}