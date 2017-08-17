//
// Created by Janusz Majchrzak on 17/08/2017.
//

#ifndef DDS_MASTEREXCEPTIONS_HPP
#define DDS_MASTEREXCEPTIONS_HPP

#include <string>
#include <stdexcept>

struct MasterException : public std::runtime_error {
    using std::runtime_error::runtime_error;
    MasterException(const std::string& msg) : std::runtime_error{msg} {
        ;
    }
};

struct MasterNotInitedException : public MasterException {
    MasterNotInitedException(const std::string& msg) : MasterException{msg} {
        ;
    }
};

struct MasterCollectSlaveInfoException : public MasterException {
    MasterCollectSlaveInfoException(const std::string& msg) : MasterException{msg} {
        ;
    }
};

#endif //DDS_MASTEREXCEPTIONS_HPP
