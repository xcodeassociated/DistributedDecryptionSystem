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
    static int message_polling_timeout;

public:

    static boost::atomic<message_id_type> id;

    Gateway(boost::shared_ptr<mpi::communicator>);

    static int get_message_polling_timeout();
    static void set_message_polling_timeout(int);

    void send(const int rank, const int tag,const MpiMessage &msg);
    boost::optional<MpiMessage> receive(const int rank, const int tag);
};


#endif //DDS_GATEWAYBASE_HPP
