//
// Created by Janusz Majchrzak on 16/08/2017.
//

#include "MasterGateway.hpp"

MasterGateway::MasterGateway(boost::shared_ptr<mpi::communicator> _world) :
        Gateway(_world) {
    ;
}

void MasterGateway::send_to_salve(const MpiMessage& msg) {
    this->send(msg.receiver, msg.sender, msg);
}

MpiMessage MasterGateway::receive_from_slave(int rank) {
    return this->receive(rank, 0);
}