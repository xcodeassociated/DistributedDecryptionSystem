//
// Created by Janusz Majchrzak on 17/08/2017.
//

#ifndef DDS_SLAVEMESSAGEHELPER_HPP
#define DDS_SLAVEMESSAGEHELPER_HPP

#include <boost/container/map.hpp>
#include <cstdint>
#include <string>
#include <MPIMessage.hpp>

struct SlaveMessageHelper {
    static MpiMessage create_INFO_CALLBACK(int, const std::string&, int);
    static MpiMessage create_INIT_CALLBACK(int, int);
    static MpiMessage create_PING_CALLBACK(int, const std::string&, int);
    static MpiMessage create_FOUND(int, const std::string&);
};


#endif //DDS_SLAVEMESSAGEHELPER_HPP
