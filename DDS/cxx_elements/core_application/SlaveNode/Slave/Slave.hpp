//
// Created by jm on 22.03.17.
//

#ifndef DDS_SLAVEMAINCLASS_HPP
#define DDS_SLAVEMAINCLASS_HPP

#include <boost/thread.hpp>
#include <boost/container/vector.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/mpi.hpp>
#include <boost/atomic.hpp>

#include <common/Message/MPIMessage.hpp>
#include <Logger.hpp>
#include <Decryptor.hpp>
#include <Gateway.hpp>

namespace mpi = boost::mpi;

class SlaveGateway : public Gateway {
public:
    using Gateway::Gateway;
    SlaveGateway(boost::shared_ptr<mpi::communicator>);
};

class Slave {
    boost::shared_ptr<Logger> logger;
    boost::shared_ptr<mpi::communicator> world = nullptr;
    SlaveGateway messageGateway;
    boost::container::vector<boost::shared_ptr<boost::thread>> thread_array;
    boost::container::vector<boost::shared_ptr<Decryptor>> worker_pointers;

public:
    Slave(boost::shared_ptr<mpi::communicator>);

    bool init();
    void start();
};

#endif //DDS_SLAVEMAINCLASS_HPP
