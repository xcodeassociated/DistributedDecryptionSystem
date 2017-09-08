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
#include <functional>

#include <SysCom.hpp>
#include <Logger.hpp>
#include <Decryptor.hpp>
#include <GatewayExceptions.hpp>
#include "SlaveGateway.hpp"
#include "SlaveMessageHelper.hpp"
#include "SlaveExceptions.hpp"
#include "Slave.hpp"

Slave::Slave(boost::shared_ptr<mpi::communicator> _world, std::string _hosts_file, std::string _encrypted_file, std::string _decrypted_file) :
        world{_world},
        hosts_file{_hosts_file},
        encrypted_file{_encrypted_file},
        decrypted_file{_decrypted_file},
        messageGateway(new SlaveGateway(this->world, hosts_file)),
        rank{this->world->rank()}{

    this->logger = Logger::instance("Slave_" + std::to_string(this->rank));
    this->logger_error = LoggerError::instance("Slave_" + std::to_string(this->rank) + "_ERROR");
}

std::string Slave::convert_progress_to_string(const boost::container::map<int, std::string> &data) {
    std::string str;
    std::stringstream ss;
    for (const auto& pair : data) {
        ss << pair.first << ":" << pair.second << std::endl;
    }
    str = ss.str();
    return str;
}

int Slave::get_available_threads() {
    return boost::thread::hardware_concurrency() - 1;
}

Slave::key_ranges Slave::convert_init_data(const std::string& data) {
    key_ranges ranges;
    std::istringstream f(data);
    std::string line;
    while (std::getline(f, line)) {
        if (line == "\n")
            break;

        boost::container::vector<std::string> strs;
        boost::split(strs, line, boost::is_any_of(","));
        assert(strs.size() == 2);
        ranges.emplace_back(boost::lexical_cast<uint64_t>(strs[0]), boost::lexical_cast<uint64_t>(strs[1]));
    }
    return ranges;
}

void Slave::init_and_start_workers() {
    int i = 0;
    for (const auto& range : this->work_ranges){
        boost::shared_ptr<boost::lockfree::spsc_queue<SysComSTR>> rx = boost::make_shared<boost::lockfree::spsc_queue<SysComSTR>>(128);
        boost::shared_ptr<boost::lockfree::spsc_queue<SysComSTR>> tx = boost::make_shared<boost::lockfree::spsc_queue<SysComSTR>>(128);

        auto ef = this->encrypted_file, df = this->decrypted_file;
        auto worker_thread = [&range, ef, df, i, rx, tx] {
            Decryptor decryptor(rx, tx);
            uint64_t rbegin = range.first;
            uint64_t rend = range.second;
            KeyRange kr = KeyRange{rbegin, rend};
            decryptor.init(i, kr, ef, df);
            decryptor.start();
        };
        boost::shared_ptr<boost::thread> worker_thread_ptr = boost::make_shared<boost::thread>(worker_thread);
        this->thread_array.push_back(worker_thread_ptr);
        this->syscom_array.push_back({rx, tx});
        i++;
    }
    this->work = true;
}

void Slave::respond_collect_info() {

    boost::optional<MpiMessage> request = this->messageGateway->receive_from_master();
    if (request.is_initialized()){
        if (request){
            if ((*request).event == MpiMessage::Event::INFO) {

                int threads = this->get_available_threads();
                MpiMessage response_msg = SlaveMessageHelper::create_INFO_CALLBACK(this->rank, std::to_string(threads), (*request).id);
                this->messageGateway->send_to_master(response_msg);

            } else
                throw SlaveNotMachingOperationRequestException{"Incorrect message event"};
        } else
            throw SlaveRequestException{"Message initialized, but empty"};
    } else
        throw SlaveRequestNotInitializedException{"Message not initialized"};
}

Slave::key_ranges Slave::respond_collect_ranges() {
    boost::optional<MpiMessage> request = this->messageGateway->receive_from_master();
    if (request.is_initialized()){
        if (request){
            if ((*request).event == MpiMessage::Event::INIT) {

                std::string data = (*request).data;
                *logger << "Received ranges: " << data << std::endl;

                MpiMessage response_msg = SlaveMessageHelper::create_INIT_CALLBACK(this->rank, (*request).id);
                this->messageGateway->send_to_master(response_msg);

                return this->convert_init_data(data);

            } else
                throw SlaveNotMachingOperationRequestException{"Incorrect message event"};
        } else
            throw SlaveRequestException{"Message initialized, but empty"};
    } else
        throw SlaveRequestNotInitializedException{"Message not initialized"};
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
        throw SlaveNotInitedException{"Slave has not been initialized"};

    this->init_and_start_workers();

    while (this->work) {
        try {
            boost::optional<MpiMessage> request = this->messageGateway->receive_from_master();
            if (request.is_initialized()){
                if (request){


                    switch ((*request).event) {
                        case MpiMessage::Event::PING: {
                            int i = 0;

                            boost::container::map<int, std::string> info_ping{};
                            for (auto& com : this->syscom_array) {
                                com.first->push(SysComSTR{"", SysComSTR::Type::PING});

                                SysComSTR msg;
                                while (!com.second->pop(msg)) { ; }

                                if (msg.type == SysComSTR::Type::FOUND) {
                                    std::string key_str = msg.data;
                                    MpiMessage msg = SlaveMessageHelper::create_FOUND(this->rank, key_str);
                                    this->messageGateway->send_to_master(msg);
                                } else if (msg.type == SysComSTR::Type::CALLBACK) {
                                    std::string key_str = msg.data;
                                    info_ping[i++] = key_str;
                                } else { ; }

                            }

                            std::string data = this->convert_progress_to_string(info_ping);
                            MpiMessage msg = SlaveMessageHelper::create_PING_CALLBACK(this->rank, data, (*request).id);
                            this->messageGateway->send_to_master(msg);

                        }break;

                        case MpiMessage::Event::KILL: {
                            *logger << "Received KILL MSG" << std::endl;
                            for (const auto& com : this->syscom_array)
                                com.first->push(SysComSTR{"", SysComSTR::Type::KILL});

                            *logger << "Waiting for workers threads..." << std::endl;
                            for (const auto& wt : this->thread_array) {
                                if (wt->joinable())
                                    wt->join();
                            }
                            *logger << "Worker threads joined." << std::endl;

                            this->work = false;
                        }break;

                        default: {
                            throw SlaveNotMachingOperationRequestException{"Unknow MpiMessage event type"};
                        }
                    }


                } else
                    throw SlaveRequestException{"Received data was initialized but was empty"};
            } else
                continue;

        } catch (const GatewayIncorrectRankException& e) {
            *logger_error << "GatewayIncorrectRankException: " << e.what() << std::endl;

        } catch (const GatewayPingException& e) {
            *logger_error << "GatewayPingException: " << e.what() << std::endl;

        } catch (const GatewaySendException& e) {
            *logger_error << "GatewaySendException: " << e.what() << std::endl;

        } catch (const GatewayException& e) {
            *logger_error << "GatewayException: " << e.what() << std::endl;

        }
    }
}