//
// Created by Janusz Majchrzak on 17/08/2017.
//

#ifndef DDS_SLAVEEXCEPTIONS_HPP
#define DDS_SLAVEEXCEPTIONS_HPP

#include <string>
#include <stdexcept>

struct SlaveException : public std::runtime_error {
    using std::runtime_error::runtime_error;
    SlaveException(const std::string& msg) : std::runtime_error{msg} {
        ;
    }
};

#endif //DDS_SLAVEEXCEPTIONS_HPP
