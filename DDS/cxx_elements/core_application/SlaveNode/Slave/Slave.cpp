//
// Created by jm on 22.03.17.
//

#include <boost/numeric/conversion/cast.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/move/move.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/function.hpp>

#include "Slave.hpp"

SlaveGateway::SlaveGateway(boost::shared_ptr<mpi::communicator> _world) :
        Gateway(_world) {
    ;
}

Slave::Slave(boost::shared_ptr<mpi::communicator> _world) :
        world{_world},
        messageGateway{this->world} {
    std::string logger_label = "Slave_" + std::to_string(this->world->rank());
    this->logger = Logger::instance(logger_label);
}

bool Slave::init() {
    this->thread_array = {};
    this->worker_pointers = {};

    *logger << "init\n";
    return false;
}

void Slave::start(){

}