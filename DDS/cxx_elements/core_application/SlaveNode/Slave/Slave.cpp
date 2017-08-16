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

SlaveGateway::SlaveGateway(boost::shared_ptr<mpi::communicator> _world, const std::string& _hosts_file_name) :
        Gateway(_world, _hosts_file_name) {
    ;
}

void SlaveGateway::send_to_master(const MpiMessage& msg) {
    this->unsafe_send(0, 0, msg);
}

boost::optional<MpiMessage> SlaveGateway::receive_from_master() {
    return this->unsafe_receive(0, 0);
}

Slave::Slave(boost::shared_ptr<mpi::communicator> _world, std::string _hosts_file) :
        world{_world},
        hosts_file{_hosts_file},
        messageGateway{this->world, hosts_file} {

    this->logger = Logger::instance("Slave");
}

bool Slave::init() {
    this->thread_array = {};
    this->worker_pointers = {};

    *logger << "init\n";
    return false;
}

void Slave::start(){

}