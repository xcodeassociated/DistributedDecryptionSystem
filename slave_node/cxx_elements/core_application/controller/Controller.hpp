//
// Created by jm on 29.12.16.
//

#ifndef SLAVE_NODE_CONTROLLER_HPP
#define SLAVE_NODE_CONTROLLER_HPP

#include <iostream>
namespace core {
    class Controller {
    public:
        bool foo();
        
        inline bool inline_foo() {
            return true;
        }
    };
}


#endif //SLAVE_NODE_CONTROLLER_HPP
