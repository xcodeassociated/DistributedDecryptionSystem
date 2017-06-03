//
// Created by jm on 22.03.17.
//

#ifndef DDS_MASTERMAINCLASS_HPP
#define DDS_MASTERMAINCLASS_HPP

#include <MPIMessage.hpp>
#include <Logger.hpp>
#include <Watchdog.hpp>

class Master {
    Logger logger;
    Watchdog watchdog;

public:
    Master();
    bool init(void);
};

#endif //DDS_MASTERMAINCLASS_HPP
