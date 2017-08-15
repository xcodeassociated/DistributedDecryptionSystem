//
// Created by Janusz Majchrzak on 30/05/17.
//

#ifndef DDS_LOGGER_HPP
#define DDS_LOGGER_HPP

#include <string>
#include <iostream>
//#include <mutex>
//#include <memory>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

class Logger;

class LoggerFactory{
    static boost::mutex logger_sync;
public:
    static boost::shared_ptr<Logger> create_logger(const std::string);
};

class Logger{
    friend class LoggerFactory;

    std::string component_name;
    boost::mutex& mu;

    Logger(boost::mutex& mutex_ref, const std::string _component_name);

public:
    virtual ~Logger() = default;

    static boost::shared_ptr<Logger> instance(const std::string component_name);

    template <typename T>
    Logger& operator<<(const T& t){
        boost::lock_guard<boost::mutex> lock(this->mu);
        std::cout << this->component_name << ": " << t;
        return *this;
    }

};




#endif //DDS_LOGGER_HPP
