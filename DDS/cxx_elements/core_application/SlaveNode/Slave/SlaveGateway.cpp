//
// Created by Janusz Majchrzak on 17/08/2017.
//

#include "SlaveGateway.hpp"

SlaveGateway::SlaveGateway(boost::shared_ptr<mpi::communicator> _world) :
        Gateway(_world) {
    ;
}

void SlaveGateway::send_to_master(const MpiMessage& msg) {
    this->unsafe_send(0, 0, msg);
}

boost::optional<MpiMessage> SlaveGateway::receive_from_master() {
    return this->unsafe_receive(0, 0);
}