//
// Created by jm on 22.03.17.
//

#include "Slave.hpp"

Slave::Slave() : messageGateway{}, syscommGateway{}, logger{Logger::instance("Slave")}, decryptor{} {
    ;
}

bool Slave::init() {
    *logger << "init\n";

    this->decryptor.init();
    return false;
}