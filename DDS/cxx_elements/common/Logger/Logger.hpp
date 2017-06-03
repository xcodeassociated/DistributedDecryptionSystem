//
// Created by Janusz Majchrzak on 30/05/17.
//

#ifndef DDS_LOGGER_HPP
#define DDS_LOGGER_HPP

#include <string>

class Logger {
    std::string component_name;

public:
    Logger() = delete;
    Logger(std::string _component_name);
};


#endif //DDS_LOGGER_HPP
