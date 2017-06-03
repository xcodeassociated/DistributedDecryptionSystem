//
// Created by jm on 22.03.17.
//

#include "Slave.hpp"

Slave::Slave() : logger{"Slave"}, decryptor{} {
    ;
}

bool Slave::init() {
    this->decryptor.init();
    return false;
}