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
    std::string hosts_file_name = "";
    static int timeout;
    void _send(const int rank, const int tag, const MpiMessage &msg);
    boost::optional<MpiMessage> _receive(const int rank, const int tag);

public:

    static boost::atomic<message_id_type> id;

    Gateway(boost::shared_ptr<mpi::communicator>, const std::string& _hosts_file_name);

    void ping(const int rank);
    void send(const int rank, const int tag,const MpiMessage &msg);
    boost::optional<MpiMessage> receive(const int rank, const int tag);
    boost::optional<MpiMessage> send_and_receive(const int rank, const int tag, const MpiMessage &msg);
    void unsafe_send(const int rank, const int tag, const MpiMessage &msg);
    boost::optional<MpiMessage> unsafe_receive(const int rank, const int tag);
};


#endif //DDS_GATEWAYBASE_HPP
