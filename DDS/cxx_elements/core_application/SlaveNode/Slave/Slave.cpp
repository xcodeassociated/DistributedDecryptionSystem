//
// Created by jm on 22.03.17.
//

#include <boost/numeric/conversion/cast.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/optional.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/move/move.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/thread.hpp>

#include <sstream>

#include <Logger.hpp>
#include <Decryptor.hpp>
#include <GatewayExceptions.hpp>
#include "SlaveGateway.hpp"
#include "SlaveMessageHelper.hpp"
#include "SlaveExceptions.hpp"
#include "Slave.hpp"

Slave::Slave(boost::shared_ptr<mpi::communicator> _world, std::string _hosts_file) :
        world{_world},
        hosts_file{_hosts_file},
        messageGateway(new SlaveGateway(this->world, hosts_file)),
        rank{this->world->rank()}{

    this->logger = Logger::instance("Slave");
    this->logger_error = LoggerError::instance("Slave_ERROR");
}

std::string Slave::convert_progress_to_string(const boost::container::map<int, uint64_t> &data) {
    return {};
}

int Slave::get_available_threads() {
    return boost::thread::hardware_concurrency() - 1;
}

Slave::key_ranges Slave::convert_init_data(const std::string& data) {
    key_ranges ranges;
    std::istringstream f(data);
    std::string line;
    while (std::getline(f, line)) {
        boost::container::vector<std::string> strs;
        boost::split(strs, line, boost::is_any_of(","));
        assert(strs.size() == 2);
        ranges.emplace_back(boost::lexical_cast<uint64_t>(strs[0]), boost::lexical_cast<uint64_t>(strs[1]));
    }
}

void Slave::init_workers() {

}

void Slave::respond_collect_info() {

    boost::optional<MpiMessage> request = this->messageGateway->receive_from_master();
    if (request.is_initialized()){
        if (request){
            if ((*request).event == MpiMessage::Event::INFO) {

                int threads = this->get_available_threads();
                MpiMessage respond_msg = SlaveMessageHelper::create_INFO_CALLBACK(this->rank, std::to_string(threads), (*request).id);
                this->messageGateway->send_to_master(respond_msg);

            } else
                throw SlaveNotMachingOperationRequestException{""};
        } else
            throw SlaveRequestException{""};
    } else
        throw SlaveRequestNotInitializedException{""};
}

Slave::key_ranges Slave::respond_collect_ranges() {
    boost::optional<MpiMessage> request = this->messageGateway->receive_from_master();
    if (request.is_initialized()){
        if (request){
            if ((*request).event == MpiMessage::Event::INIT) {

                std::string data = (*request).data;
                *logger << "Received ranges: " << data << std::endl;

                MpiMessage respond_msg = SlaveMessageHelper::create_INIT_CALLBACK(this->rank, (*request).id);
                this->messageGateway->send_to_master(respond_msg);

                return this->convert_init_data(data);

            } else
                throw SlaveNotMachingOperationRequestException{""};
        } else
            throw SlaveRequestException{""};
    } else
        throw SlaveRequestNotInitializedException{""};
}

bool Slave::init() {

    *logger << "init" << std::endl;

    try {

        this->respond_collect_info();
        this->work_ranges = this->respond_collect_ranges();
        this->inited = true;
        return true;

    } catch (const GatewayIncorrectRankException& e) {
        *logger_error << "GatewayIncorrectRankException: " << e.what() << std::endl;

        return false;
    } catch (const GatewayPingException& e) {
        *logger_error << "GatewayPingException: " << e.what() << std::endl;

        return false;
    } catch (const GatewaySendException& e) {
        *logger_error << "GatewaySendException: " << e.what() << std::endl;

        return false;
    } catch (const GatewayException& e) {
        *logger_error << "GatewayException: " << e.what() << std::endl;

        return false;
    }

}

void Slave::start(){
    if (!inited)
        throw SlaveNotInitedException{""};

    this->init_workers();

}