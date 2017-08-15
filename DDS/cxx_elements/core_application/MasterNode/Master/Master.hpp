//
// Created by jm on 22.03.17.
//

#ifndef DDS_MASTERMAINCLASS_HPP
#define DDS_MASTERMAINCLASS_HPP

#include <memory>
#include <cmath>

#include <boost/container/vector.hpp>
#include <boost/container/map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/mpi.hpp>

#include <common/Message/MPIMessage.hpp>
#include <Logger.hpp>
#include <Gateway.hpp>

namespace mpi = boost::mpi;

class MasterGateway : public Gateway {
public:
    using Gateway::Gateway;
    MasterGateway(boost::shared_ptr<mpi::communicator>, const std::string&);

    void send_to_salve(int, const MpiMessage&);
    boost::optional<MpiMessage> receive_from_slave(int);
};

class Master {
    boost::shared_ptr<Logger> logger;
    boost::shared_ptr<mpi::communicator> world = nullptr;
    MasterGateway messageGateway;

public:
    Master(boost::shared_ptr<mpi::communicator>);
    bool init(uint64_t, uint64_t);
    boost::container::vector<std::pair<uint64_t, uint64_t>> calculate_range(uint64_t absolute_key_from, uint64_t absolute_key_to, int size);
    void collect_slave_info();
    void prepare_slaves();
    void start();
};

#endif //DDS_MASTERMAINCLASS_HPP
