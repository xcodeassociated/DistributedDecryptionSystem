//
// Created by Janusz Majchrzak on 17/08/2017.
//

#ifndef DDS_SLAVEGATEWAY_HPP
#define DDS_SLAVEGATEWAY_HPP

#include <MPIMessage.hpp>
#include <Gateway.hpp>
#include <boost/mpi.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/operators.hpp>

namespace mpi = boost::mpi;

class SlaveGateway : public Gateway {
public:
    using Gateway::Gateway;
    SlaveGateway(boost::shared_ptr<mpi::communicator>, const std::string&);

    void send_to_master(const MpiMessage&);
    boost::optional<MpiMessage> receive_from_master();
};


#endif //DDS_SLAVEGATEWAY_HPP
