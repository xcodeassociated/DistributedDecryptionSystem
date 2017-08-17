//
// Created by jm on 22.03.17.
//

#ifndef DDS_MASTERMAINCLASS_HPP
#define DDS_MASTERMAINCLASS_HPP

#include <memory>
#include <cmath>
#include <stdexcept>
#include <string>

#include <boost/container/vector.hpp>
#include <boost/container/map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/mpi.hpp>

namespace mpi = boost::mpi;

class Logger;
class LoggerError;
class MasterGateway;
class JsonFileOperations;

class Master {
public:

    using key_ranges = boost::container::vector<std::pair<uint64_t, uint64_t>>;
    using slave_info = boost::container::map<int, int>;

private:

    boost::shared_ptr<Logger> logger = nullptr;
    boost::shared_ptr<LoggerError> logger_error = nullptr;
    boost::shared_ptr<mpi::communicator> world = nullptr;
    std::string hosts_file = "";
    std::string progress_file = "";
    boost::shared_ptr<MasterGateway> messageGateway = nullptr;
    boost::shared_ptr<JsonFileOperations> jsonFile = nullptr;

    bool work = false;
    bool inited = false;

    boost::container::map<int, key_ranges> progress;

    enum class Fault_Type : int {
        PING_FAULT = 1,
        SEND_FAULT,
        RECEIVE_FAULT
    };

    void fault_handle(int, Fault_Type);
    int get_slaves_count() const;
    slave_info collect_slave_info();
    void init_slaves(slave_info&, const key_ranges&);
    boost::container::map<int, uint64_t> convert_ping_report(const std::string&);

public:

    Master(boost::shared_ptr<mpi::communicator>, std::string, std::string);
    bool init(uint64_t, uint64_t);
    bool init(const std::string&);
    key_ranges calculate_range(uint64_t absolute_key_from, uint64_t absolute_key_to, int size) const;

    void start();
};

#endif //DDS_MASTERMAINCLASS_HPP
