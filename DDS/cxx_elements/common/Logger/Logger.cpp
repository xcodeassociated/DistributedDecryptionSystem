//
// Created by Janusz Majchrzak on 30/05/17.
//

#include <boost/make_shared.hpp>
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "Logger.hpp"

boost::mutex LoggerFactory::logger_sync;

Logger::Logger(boost::mutex& mutex_ref, const std::string _component_name) :
        mu{mutex_ref}, component_name{_component_name}{
};

boost::shared_ptr<Logger> Logger::instance(const std::string component_name){
    return LoggerFactory::create_logger(component_name);
}

std::string Logger::getTimestamp() {
    boost::posix_time::ptime posix_time = boost::posix_time::microsec_clock::local_time();
    return boost::posix_time::to_simple_string(posix_time);
}

boost::shared_ptr<Logger> LoggerFactory::create_logger(const std::string component_name){
    return boost::shared_ptr<Logger>(new Logger(LoggerFactory::logger_sync, component_name));
}

boost::shared_ptr<LoggerError> LoggerFactory::create_logger_error(const std::string component_name){
    return boost::shared_ptr<LoggerError>(new LoggerError(LoggerFactory::logger_sync, component_name));
}

LoggerError::LoggerError(boost::mutex& mutex_ref, const std::string _component_name) : Logger(mutex_ref, _component_name) {
    ;
}

boost::shared_ptr<LoggerError> LoggerError::instance(const std::string component_name) {
    return LoggerFactory::create_logger_error(component_name);
}
