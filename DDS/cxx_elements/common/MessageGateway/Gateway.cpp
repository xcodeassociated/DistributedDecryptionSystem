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
#include "GatewayExceptions.hpp"

int Gateway::timeout = 3000000;
boost::atomic<message_id_type> Gateway::id{0};

Gateway::Gateway(boost::shared_ptr<mpi::communicator> _world) :
        world{_world} {
    ;
}

void Gateway::_send(const int rank, const int tag, const MpiMessage &msg){
    boost::posix_time::ptime begin = boost::posix_time::microsec_clock::local_time();
    mpi::request send_request = world->isend(rank, tag, msg);
    while (true){
        if (send_request.test())
            break;

        boost::posix_time::ptime end = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::time_duration duration = end - begin;
        if (duration.total_microseconds() >= timeout)
            throw GatewaySendException{"Send timeout"};
    }
}

boost::optional<MpiMessage> Gateway::_receive(const int rank, const int tag){
    MpiMessage data;
    boost::posix_time::ptime begin_probe = boost::posix_time::microsec_clock::local_time();
    while (true) {
        boost::optional<mpi::status> stat = world->iprobe(rank, tag);
        if (stat) {
            boost::posix_time::ptime begin_receive = boost::posix_time::microsec_clock::local_time();
            mpi::request recv_request = world->irecv(stat->source(), stat->tag(), data);
            while (true) {
                if (recv_request.test())
                    return boost::optional<MpiMessage>{data};

                boost::posix_time::ptime end_receive = boost::posix_time::microsec_clock::local_time();
                boost::posix_time::time_duration duration_receive = end_receive - begin_receive;
                if (duration_receive.total_microseconds() >= timeout)
                    throw GatewayReceiveException{"Receive timeout"};

            }
        }
        boost::posix_time::ptime end_probe = boost::posix_time::microsec_clock::local_time();
        boost::posix_time::time_duration duration_probe = end_probe - begin_probe;

        if (duration_probe.total_microseconds() >= timeout)
            break;
    }
    return {};
}


void Gateway::send(const int rank, const int tag, const MpiMessage &msg){
    _send(rank, tag, msg);
}

boost::optional<MpiMessage> Gateway::receive(const int rank, const int tag) {
    return _receive(rank, tag);
}

boost::optional<MpiMessage> Gateway::send_and_receive(const int rank, const int tag, const MpiMessage &msg) {
    _send(rank, tag, msg);
    return _receive(rank, tag);
}

void Gateway::unsafe_send(const int rank, const int tag, const MpiMessage &msg){
    _send(rank, tag, msg);
}

boost::optional<MpiMessage> Gateway::unsafe_receive(const int rank, const int tag) {
    return _receive(rank, tag);
}
