//
// Created by jm on 22.03.17.
//

#ifndef DDS_MASTERMAINCLASS_HPP
#define DDS_MASTERMAINCLASS_HPP

#include <memory>

#include <MPIMessage.hpp>
#include <Logger.hpp>
#include <Watchdog.hpp>
#include "MasterMessageGateway.hpp"

class Master {
    std::shared_ptr<Logger> logger;
    MasterMessageGateway messageGateway;
    Watchdog watchdog;

public:
    Master();
    bool init(void);
};

#endif //DDS_MASTERMAINCLASS_HPP
