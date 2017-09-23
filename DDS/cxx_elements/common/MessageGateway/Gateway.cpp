//
// Created by Janusz Majchrzak on 03/08/2017.
//

#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <sstream>
#include <utility>
#include <fstream>

#include "Gateway.hpp"

boost::atomic<message_id_type> Gateway::id{0};
int Gateway::message_polling_timeout = 0;

Gateway::Gateway(boost::shared_ptr<mpi::communicator> _world) : world{_world} {
    assert(Gateway::message_polling_timeout > 0);
}

int Gateway::get_message_polling_timeout() {
    return Gateway::message_polling_timeout;
}

void Gateway::set_message_polling_timeout(int _timeout) {
    assert(_timeout > 0);
    Gateway::message_polling_timeout = _timeout;
}

boost::optional<MpiMessage> Gateway::receive(const int rank, const int tag){
    MpiMessage data;
    boost::posix_time::ptime begin_probe = boost::posix_time::microsec_clock::local_time();
    while (true) {
        boost::optional<mpi::status> stat = world->iprobe(rank, tag);
        if (stat) {
            world->recv(stat->source(), stat->tag(), data);
            return boost::optional<MpiMessage>{data};
        }
        boost::posix_time::ptime end_probe = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::time_duration duration_probe = end_probe - begin_probe;

        if (duration_probe.total_microseconds() >= Gateway::message_polling_timeout)
            break;
    }
    return {};
}


void Gateway::send(const int rank, const int tag, const MpiMessage &msg){
    world->send(rank, tag, msg);
}

