//
// Created by Janusz Majchrzak on 03/08/2017.
//

#ifndef DDS_GATEWAYBASE_HPP
#define DDS_GATEWAYBASE_HPP

#include <stdexcept>

#include <boost/mpi.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/atomic.hpp>

#include <MPIMessage.hpp>

namespace mpi = boost::mpi;

class Gateway {
protected:

    boost::shared_ptr<mpi::communicator> world;

public:

    static boost::atomic<message_id_type> id;

    Gateway(boost::shared_ptr<mpi::communicator>);

    void send(const int rank, const int tag,const MpiMessage &msg);
    MpiMessage receive(const int rank, const int tag);
    MpiMessage send_and_receive(const int rank, const int tag, const MpiMessage &msg);
};


#endif //DDS_GATEWAYBASE_HPP
