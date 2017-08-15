//
// Created by jm on 22.03.17.
//

#ifndef DDS_SLAVEMAINCLASS_HPP
#define DDS_SLAVEMAINCLASS_HPP

#include <common/Message/MPIMessage.hpp>
#include <Logger.hpp>
#include <Decryptor.hpp>
#include "SlaveMessageGateway.hpp"

class Slave {
    std::shared_ptr<Logger> logger;
    SlaveMessageGateway messageGateway;
    Decryptor decryptor;

public:
    Slave();

    bool init(void);
};

#endif //DDS_SLAVEMAINCLASS_HPP
