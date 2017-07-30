//
// Created by Janusz Majchrzak on 30/05/17.
//

#ifndef DDS_LOGGER_HPP
#define DDS_LOGGER_HPP

#include <string>
#include <iostream>
#include <mutex>
#include <memory>

class Logger;

class LoggerFactory{
    static std::mutex logger_sync;
public:
    static std::shared_ptr<Logger> create_logger(const std::string);
};

class Logger{
    friend class LoggerFactory;

    std::string component_name;
    std::mutex& mu;

    Logger(std::mutex& mutex_ref, const std::string _component_name);

public:
    virtual ~Logger() = default;

    static std::shared_ptr<Logger> instance(const std::string component_name);

    template <typename T>
    Logger& operator<<(const T& t){
        //std::lock_guard<std::mutex> lock(this->mu);
        std::cout << this->component_name << ": " << t;
        return *this;
    }

};




#endif //DDS_LOGGER_HPP
