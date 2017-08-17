//
// Created by Janusz Majchrzak on 30/05/17.
//

#ifndef DDS_LOGGER_HPP
#define DDS_LOGGER_HPP

#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

class Logger;
class LoggerError;

class LoggerFactory{
    static boost::mutex logger_sync;
public:
    static boost::shared_ptr<Logger> create_logger(const std::string);
    static boost::shared_ptr<LoggerError> create_logger_error(const std::string);
};

class Logger{
    friend class LoggerFactory;
    friend class LoggerError;

    Logger(boost::mutex& mutex_ref, const std::string _component_name);

protected:
    std::string component_name;
    boost::mutex& mu;
    std::string getTimestamp();
    bool flushed = true;

public:
    virtual ~Logger() = default;

    static boost::shared_ptr<Logger> instance(const std::string component_name);

    typedef std::basic_ostream<char, std::char_traits<char>> CoutType;
    typedef CoutType& (*StandardEndLine)(CoutType&);

    Logger& operator<<(StandardEndLine manip) {
        flushed = true;
        manip(std::cout);
        return *this;
    }

    Logger& operator<<(char c) {
        boost::lock_guard<boost::mutex> lock(this->mu);
        if (c == '\n')
            flushed = true;

        if (flushed) {
            std::cout << '[' << this->getTimestamp() << "] " << this->component_name << ": " << c;
            flushed = false;
        }else
            std::cout << c;
        return *this;
    }

    template <typename T>
    Logger& operator<<(const T& t) {
        boost::lock_guard<boost::mutex> lock(this->mu);

        if (flushed) {
            std::cout << '[' << this->getTimestamp() << "] " << this->component_name << ": " << t;
            flushed = false;
        }else
            std::cout << t;

        return *this;
    }

};

class LoggerError : public Logger {
    friend class LoggerFactory;

    using Logger::Logger;

    LoggerError(boost::mutex& mutex_ref, const std::string _component_name);

public:
    virtual ~LoggerError() = default;

    static boost::shared_ptr<LoggerError> instance(const std::string component_name);

    LoggerError& operator<<(StandardEndLine manip) {
        flushed = true;
        manip(std::cerr);
        return *this;
    }

    LoggerError& operator<<(char c) {
        boost::lock_guard<boost::mutex> lock(this->mu);
        if (c == '\n')
            flushed = true;

        if (flushed) {
            std::cerr << '[' << this->getTimestamp() << "] " << this->component_name << ": " << c;
            flushed = false;
        }else
            std::cerr << c;
        return *this;
    }

    template <typename T>
    LoggerError& operator<<(const T& t) {
        boost::lock_guard<boost::mutex> lock(this->mu);
        if (flushed) {
            std::cerr << '[' << this->getTimestamp() << "] " << this->component_name << ": " << t;
            flushed = false;
        }else
            std::cerr << t;

        return *this;
    }

};



#endif //DDS_LOGGER_HPP
