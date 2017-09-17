//
// Created by Janusz Majchrzak on 03/08/2017.
//

#include <boost/chrono.hpp>
#include <boost/filesystem.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <sstream>
#include <utility>
#include <fstream>

#include "Gateway.hpp"

boost::atomic<message_id_type> Gateway::id{0};

Gateway::Gateway(boost::shared_ptr<mpi::communicator> _world) : world{_world} {
    ;
}

void Gateway::send(const int rank, const int tag, const MpiMessage &msg){
    world->send(rank, tag, msg);
}

MpiMessage Gateway::receive(const int rank, const int tag){
    MpiMessage data;
    mpi::status stat = world->probe(rank, tag);
    world->recv(stat.source(), stat.tag(), data);
    return data;
}


MpiMessage Gateway::send_and_receive(const int rank, const int tag, const MpiMessage &msg) {
    send(rank, tag, msg);
    return receive(rank, tag);
}

