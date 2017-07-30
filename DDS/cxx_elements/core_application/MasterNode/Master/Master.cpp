//
// Created by jm on 22.03.17.
//

#include "Master.hpp"

Master::Master() : logger{Logger::instance("Master")}, watchdog{} {
    ;
}

bool Master::init() {
    *logger << "init\n";
    return false;
}