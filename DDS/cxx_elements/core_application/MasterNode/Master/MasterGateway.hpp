//
// Created by Janusz Majchrzak on 16/08/2017.
//

#ifndef DDS_MASTERGATEWAY_HPP
#define DDS_MASTERGATEWAY_HPP

#include <MPIMessage.hpp>
#include <Gateway.hpp>
#include <boost/mpi.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/operators.hpp>

namespace mpi = boost::mpi;


class MasterGateway : public Gateway {
public:

    MasterGateway(boost::shared_ptr<mpi::communicator>);

    void send_to_salve(const MpiMessage&);
    MpiMessage receive_from_slave(int);
};


#endif //DDS_MASTERGATEWAY_HPP
