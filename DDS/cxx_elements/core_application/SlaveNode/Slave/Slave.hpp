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
#include <boost/lockfree/spsc_queue.hpp>
#include <string>

namespace mpi = boost::mpi;

class Logger;
class LoggerError;
class SlaveGateway;
class Decryptor;

template <typename T>
struct SysCom;

class Slave {
    using key_ranges = boost::container::vector<std::pair<uint64_t, uint64_t>>;
    using SysComSTR = SysCom<std::string>;

    boost::shared_ptr<Logger> logger = nullptr;
    boost::shared_ptr<LoggerError> logger_error = nullptr;
    boost::shared_ptr<mpi::communicator> world = nullptr;
    std::string encrypted_file = "";
    std::string decrypted_file = "";
    boost::shared_ptr<SlaveGateway> messageGateway = nullptr;
    boost::container::vector<boost::shared_ptr<boost::thread>> thread_array;
    boost::container::vector<std::pair<
            boost::shared_ptr<boost::lockfree::spsc_queue<SysComSTR>>, boost::shared_ptr<boost::lockfree::spsc_queue<SysComSTR>>
    >> syscom_array;

    key_ranges work_ranges;
    int rank = 0;
    bool inited = false;
    bool work = false;

    std::string convert_progress_to_string(const boost::container::map<int, std::string> &);
    int get_available_threads();
    void respond_collect_info();
    key_ranges respond_collect_ranges();
    key_ranges convert_init_data(const std::string&);
    void init_and_start_workers();

public:
    Slave(boost::shared_ptr<mpi::communicator>, std::string, std::string);

    bool init();
    void start();
};

#endif //DDS_SLAVEMAINCLASS_HPP
