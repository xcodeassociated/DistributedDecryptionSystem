//
// Created by jm on 29.12.16.
//

#ifndef SLAVE_NODE_CONTROLLER_HPP
#define SLAVE_NODE_CONTROLLER_HPP

#include <iostream>

class Controller {
public:
    void foo();
    inline void inline_foo() {
        std::cout << "inline foo" << "\n";
    }
};


#endif //SLAVE_NODE_CONTROLLER_HPP
