//
// Created by Janusz Majchrzak on 03/08/2017.
//

#ifndef DDS_GATEWAYBASE_HPP
#define DDS_GATEWAYBASE_HPP

#include <boost/mpi.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

#include <MPIMessage.hpp>

namespace mpi = boost::mpi;

class Gateway {
protected:
    boost::shared_ptr<mpi::communicator> world;

    void _send(const int rank, const int tag, const std::string &msg);
    boost::optional<std::string> _receive(const int rank, const int tag);
public:

    Gateway(boost::shared_ptr<mpi::communicator>);

    void ping(const int rank);
    void send(const int rank, const int tag, const std::string &msg);
    boost::optional<std::string> receive(const int rank, const int tag);
    boost::optional<std::string> send_and_receive(const int rank, const int tag, const std::string &msg);
    void unsafe_send(const int rank, const int tag, const std::string &msg);
    boost::optional<std::string> unsafe_receive(const int rank, const int tag);
};


#endif //DDS_GATEWAYBASE_HPP
