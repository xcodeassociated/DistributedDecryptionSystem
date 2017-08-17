//
// Created by Janusz Majchrzak on 16/08/2017.
//

#ifndef DDS_MESSAGEHELPER_HPP
#define DDS_MESSAGEHELPER_HPP

#include <MPIMessage.hpp>
#include <cstdint>

struct MessageHelper {
    static MpiMessage create_INFO(int);
    static MpiMessage create_INIT(int, uint64_t, uint64_t);
    static MpiMessage create_PING(int);
    static MpiMessage create_KILL(int);
};



#endif //DDS_MESSAGEHELPER_HPP
