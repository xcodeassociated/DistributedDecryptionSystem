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

#include "Master.hpp"

MasterGateway::MasterGateway(boost::shared_ptr<mpi::communicator> _world) :
        Gateway(_world) {
    ;
}

Master::Master(boost::shared_ptr<mpi::communicator> _world) :
        world{_world},
        logger{Logger::instance("Master")},
        messageGateway{this->world} {
    ;
}

boost::container::vector<std::pair<uint64_t, uint64_t>> Master::calculate_range(uint64_t absolute_key_from, uint64_t absolute_key_to, int size) {
    assert(absolute_key_from < absolute_key_to);
    uint64_t range = absolute_key_to - absolute_key_from;
    double ratio = static_cast<double>(range) / static_cast<double>(size);
    boost::container::vector<std::pair<uint64_t, uint64_t>> ranges{};

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

bool Master::init(uint64_t range_begine, uint64_t range_end) {
    *logger << "init\n";
    return false;
}

void Master::collect_slave_info(){

}

void Master::prepare_slaves(){

}

void Master::start(){

}
