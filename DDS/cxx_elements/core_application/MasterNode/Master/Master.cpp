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
#include <boost/function.hpp>
#include <boost/regex.hpp>

#include <sstream>
#include <fstream>
#include <algorithm>
#include <signal.h>
#include <unistd.h>

#include <Logger.hpp>
#include <FileOperation.cpp>
#include <FileCheck.cpp>
#include "MasterMessageHelper.hpp"
#include "MasterGateway.hpp"
#include "MasterExceptions.hpp"
#include "Master.hpp"

Master::Master(boost::shared_ptr<mpi::communicator> _world, std::string _progress_file) :
        world{_world},
        logger{Logger::instance("Master")},
        logger_error{LoggerError::instance("Master_ERROR")},
        progress_file{_progress_file},
        messageGateway(new MasterGateway(this->world))  {
    ;
}

Master::key_ranges Master::calculate_range(uint64_t absolute_key_from, uint64_t absolute_key_to, int size) const {
    assert(absolute_key_from < absolute_key_to);
    uint64_t range = absolute_key_to - absolute_key_from;
    double ratio = static_cast<double>(range) / static_cast<double>(size);
    Master::key_ranges ranges{};

    uint64_t begin = 0, next = 0;
    for (int i = 0; i < size; i++){
        if (i == 0) {
            begin = absolute_key_from;
            next = begin + static_cast<uint64_t>(ratio);
        } else {
            begin = next + 1;
            next = next + static_cast<uint64_t>(ratio);
        }

        ranges.emplace_back(begin, next);
    }

    if (std::floor(ratio) != ratio) {
        double integer_part, decimal_part = std::modf(ratio, &integer_part);
        double precission_diff = decimal_part * static_cast<double>(size);
        double compensation = std::ceil(precission_diff);
        ranges.back() = {ranges.back().first, (ranges.back().second + static_cast<uint64_t>(compensation))};
    }

    if (ranges.back().second > absolute_key_to)
        ranges.back().second = absolute_key_to;

    return ranges;
};


void Master::update_progress(int rank, const boost::container::map<int, uint64_t>& info) {
    auto& kranges = this->progress[rank];
    assert(kranges.size() == info.size());

    for (const auto& pair : info) {
        kranges[pair.first] = {pair.second, kranges[pair.first].second};
    }
}

void Master::init_progress_map(int rank, const boost::container::vector<std::pair<uint64_t, uint64_t>>& initial_range) {
    key_ranges kr;
    for (const auto& pair : initial_range){
        kr.push_back({pair.first, pair.second});
    }
    this->progress[rank] = boost::move(kr);
}

Master::key_ranges Master::load_progress(const std::string& data) {
    boost::container::vector<std::string> file_lines;
    boost::split(file_lines, data, boost::is_any_of("\n"));
    file_lines.pop_back();
    key_ranges k_ranges;
    for (const auto& line : file_lines){
        std::string line_data = line;
        boost::regex expression{"\\{(.*?)\\/?\\/(.*?)\\}"}; /*   \{(.*?)\/?\/(.*?)\}   */
        const boost::sregex_iterator end;
        for (boost::sregex_iterator it(line_data.begin(), line_data.end(), expression); it != end; ++it) {
            k_ranges.emplace_back(boost::lexical_cast<uint64_t>((*it)[1]), boost::lexical_cast<uint64_t>((*it)[2]));
        }
    }

    return k_ranges;
}

boost::container::map<int, uint64_t> Master::convert_ping_report(const std::string& str) {
    //Ping format:
    //0:<key>
    //1:<key>
    //...
    //n:<key>

    boost::container::map<int, uint64_t> data{};
    std::istringstream f(str);
    std::string line;
    while (std::getline(f, line)) {
        if (line == "\n")
            break;

        boost::container::vector<std::string> strs;
        boost::split(strs, line, boost::is_any_of(":"));
        assert(strs.size() == 2);
        int th = boost::lexical_cast<int>(strs[0]);
        uint64_t key = boost::lexical_cast<uint64_t>(strs[1]);
        data[th] = key;
    }
    return data;
};

int Master::get_world_size() const {
    return this->world->size();
}

void Master::init_slaves(slave_info &si, const key_ranges& ranges) {
    *logger << "Sending work ranges to slaves" << std::endl;
    int index = 0;
    for (const auto& slave : si) {
        *logger << "Slave: " << slave.first << " -> " << slave.second << std::endl;

        std::string data;
        std::stringstream sdata;
        key_ranges e{};
        for (int i = 0; i < slave.second; i++) {
            *logger << "    [" << i << "]: " << "{" << ranges[index].first << ", " << ranges[index].second << "}"
                    << std::endl;
            sdata << ranges[index].first << "," << ranges[index].second << std::endl;

            e.push_back({ranges[index].first, ranges[index].second});

            index++;
        }
        data = sdata.str();

        //init progress map for this slave
        this->init_progress_map(slave.first, e);

        auto msg = MessageHelper::create_INIT(slave.first + 1, data);
        this->messageGateway->send_to_salve(msg);
        boost::optional<MpiMessage> response = this->messageGateway->receive_from_slave(slave.first + 1);
        if (response.is_initialized()) {
            if (response) {
                if ((*response).event != MpiMessage::Event::CALLBACK)
                    throw MasterCallbackException{"Incorrect callback event"};

                if (!(*response).respond_to)
                    throw MasterCallbackException{"Missing response"};

                if ((*(*response).respond_to).message_id != msg.id)
                    throw MasterCallbackException{"Not matching response message id"};
            }
        }

    }
}

bool Master::init(uint64_t range_begine, uint64_t range_end) {
    *logger << "Init begins, slaves: " << this->get_world_size() - 1 << std::endl;

    try {
        slave_info slaves = this->collect_slave_info();

        int total_threads = 0;
        for (const auto& pair : slaves) {
            total_threads += pair.second;
        }

        *logger << "Total threads: " << total_threads << std::endl;

        key_ranges calculated_ranges = this->calculate_range(range_begine, range_end, total_threads);

        this->init_slaves(slaves, calculated_ranges);
        this->inited = true;
        return this->inited;

    } catch (const MasterCollectSlaveInfoException& e) {
        *logger_error << "MasterCollectSlaveInfoException: " << e.what() << std::endl;

        return false;
    }catch (const MasterException& e) {
        *logger_error << "MasterException: " << e.what() << std::endl;

        return false;
    } catch (const std::runtime_error& e) {
        *logger_error << "Other std::runtime_error: " << e.what() << std::endl;

        return false;
    }
}



bool Master::init(const std::string& file_name) {
    *logger << "Init begins from file: " << file_name << ", slaves: " << this->get_world_size() - 1 << std::endl;

    try {
        slave_info slaves = this->collect_slave_info();

        int total_threads = 0;
        for (const auto& pair : slaves) {
            total_threads += pair.second;
        }

        *logger << "Total threads: " << total_threads << std::endl;

        std::ifstream ifile(file_name, std::ios::binary);
        std::stringstream read_stream;
        read_stream << ifile.rdbuf();
        key_ranges ranges = this->load_progress(read_stream.str());

        if (total_threads != ranges.size())
            throw MasterResumeException{"World size is different: " + std::to_string(this->get_world_size() - 1)};

        this->init_slaves(slaves, ranges);
        this->inited = true;
        return this->inited;
    } catch (const MasterCollectSlaveInfoException& e) {
        *logger_error << "MasterCollectSlaveInfoException: " << e.what() << std::endl;

        return false;
    }catch (const MasterException& e) {
        *logger_error << "MasterException: " << e.what() << std::endl;

        return false;
    } catch (const std::runtime_error& e) {
        *logger_error << "Other std::runtime_error: " << e.what() << std::endl;

        return false;
    }

}

Master::slave_info Master::collect_slave_info() {
    slave_info data{};
    for (int i = 1; i < this->get_world_size(); i++) {
        const auto msg = MessageHelper::create_INFO(i);

        this->messageGateway->send_to_salve(msg);
        boost::optional<MpiMessage> response = this->messageGateway->receive_from_slave(i);
        if (response.is_initialized()) {
            if (response) {
                if ((*response).event != MpiMessage::Event::CALLBACK)
                    throw MasterCallbackException{"Incorrect Callback"};

                if (!(*response).respond_to)
                    throw MasterCallbackException{"Callback not inited"};

                if ((*(*response).respond_to).message_id != msg.id)
                    throw MasterCallbackException{"Incorrect message id for Callback"};

                std::string tmp = (*response).data;
                int threads = std::atoi(tmp.c_str());

                *logger << "Received info from: [" << (*response).sender << "]:  " << threads << std::endl;

                data[i-1] = threads;
            } else
                throw MasterCollectSlaveInfoException{"Response initialized but empty"};
        } else
            throw MasterCollectSlaveInfoException{"Response not initialized"};
    }
    return data;
}

void Master::kill_all_slaves() {
    for (int i = 1; i < this->world->size(); i++){
        auto msg = MessageHelper::create_KILL(i);
        this->messageGateway->send_to_salve(msg);
    }
}

void Master::check_if_slave_done() {
    for (const auto& slave_info : this->progress) {
        if (std::find(this->slaves_done.begin(), this->slaves_done.end(), slave_info.first) != this->slaves_done.end())
            continue;

        bool done = true;
        for (const auto& slave_range : slave_info.second) {
            if (slave_range.first != slave_range.second)
                done = false;
        }
        if (done)
            this->slaves_done.push_back(slave_info.first);

    }
}

void Master::print_progress() {
    *logger << "Progress: " << std::endl;
    for (const auto& e : this->progress) {
        *logger << "[" << e.first << "]: ";
        for (const auto& range : e.second) {
            *logger << "{" << range.first << "/" << range.second << "} ";
        }
        *logger << std::endl;
    }
}

boost::container::map<int, Master::key_ranges> Master::get_progress() const {
    return this->progress;
}

void Master::start() {
    if (!this->inited)
        throw MasterNotInitedException{"Master Not initialized"};

    if (this->work)
        throw MasterException{"Already running"};

    this->work = true;

    while (this->work) {
        boost::this_thread::sleep(boost::posix_time::seconds(this->refresh_rate));

        for (int i = 1; (i < this->world->size() && this->work); i++) {
            if (this->slaves_done.size() == this->progress.size()) {
                *logger << "ALL SLAVES DONE!" << std::endl;
                this->kill_all_slaves();
                this->work = false;

            } else if (std::find(this->slaves_done.begin(), this->slaves_done.end(), (i - 1)) !=
                       this->slaves_done.end()) {
                continue;
            }

            *logger << "Sending PING to: " << i - 1 << std::endl;

            auto msg = MessageHelper::create_PING(i);
            this->messageGateway->send_to_salve(msg);
            boost::optional<MpiMessage> response = this->messageGateway->receive_from_slave(i);
            if (response.is_initialized()) {
                if (response) {

                    switch ((*response).event) {
                        case MpiMessage::Event::FOUND: {
                            uint64_t key = boost::lexical_cast<uint64_t>((*response).data);

                            *logger << "~~~ Found key by: " << i - 1 << " - " << key << " ~~~~" << std::endl;

                            this->kill_all_slaves();
                            this->work = false;

                        }break;

                        case MpiMessage::Event::CALLBACK: {
                            if ((*(*response).respond_to).event == MpiMessage::Event::PING) {
                                std::string ping_data = (*response).data;

                                if (ping_data.length() == 0)
                                    throw MasterMissingProgressReportException{
                                            "No Progress info, slave: " + std::to_string(i)};

                                auto update_data = this->convert_ping_report(ping_data);
                                this->update_progress(i - 1, update_data);

                                this->check_if_slave_done();

                            } else {
                                throw MasterMissingProgressReportException{"Incorrect message callback"};
                            }
                        }break;

                        default: { ;
                        }
                    }


                } else
                    throw MasterMissingProgressReportException{
                            "Slave Response initialized but empty, slave: " + std::to_string(i - 1)};
            } else
                throw MasterMissingProgressReportException{
                        "Slave Response not initialized (probably did not received), slave: " + std::to_string(i - 1)};
        }

        this->print_progress();
        this->dump_progress();
    }
}

void Master::dump_progress() {
    /* [0]: {a, b} ...
     *  ...
     * [n]: {n, m} ...
     * */

    std::string temp_file = "tmp_" + File::get_file_name(this->progress_file);
    std::string progress_file_path = File::get_file_path(this->progress_file);
    std::string temp_progress_file = File::make_path(progress_file_path, temp_file);
    File::check_open(temp_progress_file);

    std::ofstream file(temp_progress_file);
    for (const auto& e : this->progress) {
        file << "[" << e.first << "]: ";
        for (const auto& range : e.second) {
            file << "{" << range.first << "/" << range.second << "} ";
        }
        file << std::endl;
    }
    file.flush();
    file.close();

    File::rename(temp_progress_file, this->progress_file);

    *logger << "Progress dumped to file: " << this->progress_file << std::endl;
}