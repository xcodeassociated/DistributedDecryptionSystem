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

class Master {
public:

    using key_ranges = boost::container::vector<std::pair<uint64_t, uint64_t>>;
    using slave_info = boost::container::map<int, int>;

private:

    boost::shared_ptr<mpi::communicator> world = nullptr;
    boost::shared_ptr<Logger> logger = nullptr;
    boost::shared_ptr<LoggerError> logger_error = nullptr;
    std::string progress_file = "";
    boost::shared_ptr<MasterGateway> messageGateway = nullptr;

    bool work = false;
    bool inited = false;

    static int slave_polling_rate;

    boost::container::map<int, key_ranges> progress;
    boost::container::vector<int> slaves_done;

    int get_world_size() const;
    slave_info collect_slave_info();
    void init_slaves(slave_info&, const key_ranges&);
    boost::container::map<int, uint64_t> convert_ping_report(const std::string&);
    void update_progress(int, const boost::container::map<int, uint64_t>&);
    void init_progress_map(int, const key_ranges&);
    key_ranges calculate_range(uint64_t absolute_key_from, uint64_t absolute_key_to, int size) const;
    void kill_all_slaves();
    void check_if_slave_done();
    key_ranges load_progress(const std::string&);

public:

    Master(boost::shared_ptr<mpi::communicator>, std::string);
    ~Master();

    static int get_slave_polling_rate();
    static void set_slave_polling_rate(int rate);

    bool init(uint64_t, uint64_t);
    bool init(const std::string&);
    void start();
    void dump_progress();
    void print_progress();
    boost::container::map<int, key_ranges> get_progress() const;
};

#endif //DDS_MASTERMAINCLASS_HPP
