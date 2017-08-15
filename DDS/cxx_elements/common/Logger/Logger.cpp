//
// Created by Janusz Majchrzak on 30/05/17.
//

#include "Logger.hpp"
#include <boost/make_shared.hpp>

boost::mutex LoggerFactory::logger_sync;

Logger::Logger(boost::mutex& mutex_ref, const std::string _component_name) :
        mu{mutex_ref}, component_name{_component_name}{
};

boost::shared_ptr<Logger> Logger::instance(const std::string component_name){
    return LoggerFactory::create_logger(component_name);
}

boost::shared_ptr<Logger> LoggerFactory::create_logger(const std::string component_name){
    return boost::shared_ptr<Logger>(new Logger(LoggerFactory::logger_sync, component_name));
}
