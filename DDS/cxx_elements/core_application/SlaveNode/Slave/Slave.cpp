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

#include <Logger.hpp>
#include <Decryptor.hpp>
#include "SlaveGateway.hpp"
#include "SlaveMessageHelper.hpp"
#include "Slave.hpp"

Slave::Slave(boost::shared_ptr<mpi::communicator> _world, std::string _hosts_file) :
        world{_world},
        hosts_file{_hosts_file},
        messageGateway(new SlaveGateway(this->world, hosts_file)) {

    this->logger = Logger::instance("Slave");
    this->logger_error = LoggerError::instance("Slave_ERROR");
}

bool Slave::init() {
    this->thread_array = {};
    this->worker_pointers = {};

    *logger << "init" << std::endl;
    return false;
}

void Slave::start(){

}