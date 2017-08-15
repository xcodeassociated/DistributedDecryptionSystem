//
// Created by Janusz Majchrzak on 03/08/2017.
//

#ifndef DDS_GATEWAYBASE_HPP
#define DDS_GATEWAYBASE_HPP

#include <stdexcept>

#include <boost/mpi.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

#include <MPIMessage.hpp>

namespace mpi = boost::mpi;

struct GatewaySendException : std::runtime_error {
    using std::runtime_error::runtime_error;
    GatewaySendException(const std::string& msg) : std::runtime_error{msg} {
        ;
    }
};

struct GatewayReceiveException : std::runtime_error {
    using std::runtime_error::runtime_error;
    GatewayReceiveException(const std::string& msg) : std::runtime_error{msg} {
        ;
    }
};

struct GatewayPingException : std::runtime_error {
    using std::runtime_error::runtime_error;
    GatewayPingException(const std::string& msg) : std::runtime_error{msg} {
        ;
    }
};

struct GatewayIncorrectRankException : std::runtime_error {
    using std::runtime_error::runtime_error;
    GatewayIncorrectRankException(const std::string& msg) : std::runtime_error{msg} {
        ;
    }
};

class Gateway {
protected:
    boost::shared_ptr<mpi::communicator> world;
    std::string hosts_file_name = "";
    static int timeout;
    void _send(const int rank, const int tag, const MpiMessage &msg);
    boost::optional<MpiMessage> _receive(const int rank, const int tag);
public:

    Gateway(boost::shared_ptr<mpi::communicator>, const std::string& _hosts_file_name);

    void ping(const int rank);
    void send(const int rank, const int tag,const MpiMessage &msg);
    boost::optional<MpiMessage> receive(const int rank, const int tag);
    boost::optional<MpiMessage> send_and_receive(const int rank, const int tag, const MpiMessage &msg);
    void unsafe_send(const int rank, const int tag, const MpiMessage &msg);
    boost::optional<MpiMessage> unsafe_receive(const int rank, const int tag);
};


#endif //DDS_GATEWAYBASE_HPP
