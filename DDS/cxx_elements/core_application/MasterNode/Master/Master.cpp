//
// Created by jm on 22.03.17.
//

#include <boost/numeric/conversion/cast.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/move/move.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/function.hpp>

#include <Logger.hpp>
#include "MasterMessageHelper.hpp"
#include "MasterGateway.hpp"
#include <JsonFileOperations.hpp>
#include "GatewayExceptions.hpp"
#include "MasterExceptions.hpp"
#include "Master.hpp"

Master::Master(boost::shared_ptr<mpi::communicator> _world, std::string _hosts_file, std::string _progress_file) :
        world{_world},
        logger{Logger::instance("Master")},
        logger_error{LoggerError::instance("Master_ERROR")},
        hosts_file{_hosts_file},
        progress_file{_progress_file},
        messageGateway(new MasterGateway(this->world, hosts_file)),
        jsonFile(new JsonFileOperations("")) {
    ;
}

Master::key_ranges Master::calculate_range(uint64_t absolute_key_from, uint64_t absolute_key_to, int size) const {
    assert(absolute_key_from < absolute_key_to);
    uint64_t range = absolute_key_to - absolute_key_from;
    double ratio = static_cast<double>(range) / static_cast<double>(size);
    Master::key_ranges ranges{};

    uint64_t begin = 0, next = 0;
    for (int i = 0; i < size; i++){
        if (i == 0) {
            begin = absolute_key_from;
            next = begin + static_cast<uint64_t>(ratio);
        } else {
            begin = next + 1;
            next = next + static_cast<uint64_t>(ratio);
        }

        ranges.emplace_back(begin, next);
    }

    if (std::floor(ratio) != ratio) {
        double integer_part, decimal_part = std::modf(ratio, &integer_part);
        double precission_diff = decimal_part * static_cast<double>(size);
        double compensation = std::ceil(precission_diff);
        ranges.back() = {ranges.back().first, (ranges.back().second + static_cast<uint64_t>(compensation))};
    }

    if (ranges.back().second > absolute_key_to)
        ranges.back().second = absolute_key_to;

    return ranges;
};

int Master::get_slaves_count() const {
    return this->world->size();
}

void Master::setup_progress(const slave_info& si) {

}

bool Master::init(uint64_t range_begine, uint64_t range_end) {
    *logger << "init" << std::endl;

    try {
        slave_info slaves = this->collect_slave_info();
    } catch (const GatewayIncorrectRankException& e) {
        *logger_error << "GatewayIncorrectRankException: " << e.what() << std::endl;

        return false;
    } catch (const GatewayPingException& e) {
        *logger_error << "GatewayPingException: " << e.what() << std::endl;

        return false;
    } catch (const GatewaySendException& e) {
        *logger_error << "GatewaySendException: " << e.what() << std::endl;

        return false;
    } catch (const MasterException& e) {
        *logger_error << "MasterException: " << e.what() << std::endl;

        return false;
    } catch (const MasterCollectSlaveInfoException& e) {
        *logger_error << "MasterCollectSlaveInfoException: " << e.what() << std::endl;

        return false;
    } catch (const std::runtime_error& e) {
        *logger_error << "Other std::runtime_error: " << e.what() << std::endl;

        return false;
    }
    this->inited = true;

    return true;
}

bool Master::init(const std::string& file_name) {

}

Master::slave_info Master::collect_slave_info() {
    slave_info data{};
    for (int i = 0; i < world->size(); i++) {
        const auto msg = MessageHelper::create_INFO(i);

        this->messageGateway->send_to_salve(msg);
        boost::optional<MpiMessage> respond = this->messageGateway->receive_from_slave(i);
        if (respond) {
            std::string tmp = (*respond).data;
            int threads = boost::lexical_cast<int>(tmp);
            data[i] = threads;
        } else
            throw MasterCollectSlaveInfoException{"Empty"};
    }
    return data;
}

void Master::start() {
    if (!this->inited)
        throw MasterNotInitedException{"TODO"};

    while (true) {

    }
}

void Master::fault_handle(int rank, Master::Fault_Type fault_type) {

}