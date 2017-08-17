//
// Created by Janusz Majchrzak on 17/08/2017.
//

#ifndef DDS_RUNOPTIONSEXCEPTIONS_HPP
#define DDS_RUNOPTIONSEXCEPTIONS_HPP

#include <string>
#include <stdexcept>

struct RunOptionException : public std::runtime_error {
    using std::runtime_error::runtime_error;
    RunOptionException(const std::string& msg) : std::runtime_error{msg} {
        ;
    }
};

struct MissingParameterException : public RunOptionException {
    MissingParameterException(const std::string& msg) : RunOptionException{msg} {
        ;
    }
};

struct IncorrectParameterException : public RunOptionException {
    IncorrectParameterException(const std::string& msg) : RunOptionException{msg} {
        ;
    }
};

#endif //DDS_RUNOPTIONSEXCEPTIONS_HPP
