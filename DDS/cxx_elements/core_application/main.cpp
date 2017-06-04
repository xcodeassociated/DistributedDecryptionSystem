#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <fstream>

#include <boost/container/vector.hpp>
#include <boost/container/set.hpp>
#include <boost/container/map.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/atomic.hpp>
#include <boost/mpi.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/optional.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/move/move.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/function.hpp>

#define CRYPTO

#ifdef CRYPTO
#include <cryptopp/sha.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>
#endif

#include <Master.hpp>
#include <Slave.hpp>

namespace mpi = boost::mpi;
namespace po = boost::program_options;

using data_type = std::string;
using rank_type = int;
using message_id_type = uint32_t;

uint64_t test_match = 0;

struct MpiMessage{
    enum class Event : int {
        CALLBACK = 0,
        INIT = 1,
        PING = 2,
        FOUND = 3,
        KILL = 4,
        SLAVE_DONE = 5,
        SLAVE_REARRANGE = 6,
        SLAVE_WORKER_DONE = 7
    };

    struct Callback{
        message_id_type message_id;
        Event event;

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            ar & message_id;
            ar & event;
        }
    };

    message_id_type id;
    rank_type receiver;
    rank_type sender;
    Event event;
    bool need_respond = false;
    data_type data;
    // only for CALLBACK message
    boost::optional<Callback> respond_to;

    MpiMessage() = default;
    MpiMessage(message_id_type _id,
               rank_type _receiver,
               rank_type _sender,
               Event _event,
               bool _need_respond,
               data_type data,
               boost::optional<Callback> res = boost::none)
            : id{_id},
              receiver{_receiver},
              sender{_sender},
              event{_event},
              need_respond{_need_respond},
              data{data},
              respond_to{res}
    {}

    MpiMessage(const MpiMessage&) = default;

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & id;
        ar & receiver;
        ar & sender;
        ar & event;
        ar & need_respond;
        ar & data;
        ar & respond_to;
    }
};

class syscom_message_counter{
    boost::atomic_uint32_t counter{0};

public:
    syscom_message_counter() = default;
    syscom_message_counter(uint32_t value) : counter{value} {}

    auto operator++() -> uint32_t{
        return this->counter++;
    }
    auto operator++(int) -> uint32_t{
        return ++(this->counter);
    }
    auto get() -> uint32_t{
        return this->counter.load();
    }
    auto operator()() -> uint32_t {
        return this->get();
    }
};

struct SysComMessage{
    using DataType = std::string;

    enum class Event : int {
        CALLBACK = 0,
        KEY_FOUND = 1,
        INTERRUPT = 2,
        PING = 3,
        WORKER_DONE = 4,
        WORKER_REARRANGE = 5
    };

    struct Callback{
        message_id_type message_id;
        Event event;
    };

    message_id_type id;
    rank_type rank;
    Event event;
    bool need_respond = false;
    DataType data;
    // only for CALLBACK message
    boost::optional<Callback> respond_to;

    SysComMessage() = default;

    SysComMessage(message_id_type _id,
                  rank_type _rank,
                  Event _evt,
                  bool _need_respond,
                  DataType _data,
                  boost::optional<Callback> res = boost::none)
            : id{_id},
              rank{_rank},
              event{_evt},
              need_respond{_need_respond},
              data{_data},
              respond_to{res}
    {}

    SysComMessage(const SysComMessage&) = default;
};

class CallBackTimer {
    boost::atomic<bool> _execute;
    boost::thread _thd;
public:
    CallBackTimer()
            :_execute(false)
    {}

    ~CallBackTimer() {
        if( _execute.load(boost::memory_order_acquire) ) {
            stop();
        };
    }

    void stop() {
        _execute.store(false, boost::memory_order_release);
        if( _thd.joinable() )
            _thd.join();
    }

    template <typename Chrono>
    void start(Chrono && interval, boost::function<void(void)> func) {
        if( _execute.load(boost::memory_order_acquire) ) {
            stop();
        };
        _execute.store(true, boost::memory_order_release);
        _thd = boost::thread([this, interval, func]() {
            while (_execute.load(boost::memory_order_acquire)) {
                func();
                boost::this_thread::sleep_for(interval);
            }
        });
    }

    bool is_running() const noexcept {
        return (_execute.load(boost::memory_order_acquire) && _thd.joinable());
    }
};

class Worker{
public:
    struct KeyRange{
        uint64_t begin;
        uint64_t end;
    };

private:
    int id = 0;
    KeyRange range{0, 0};
    bool work = false;
    bool process = false;
    uint64_t current_key = 0;
    boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>> syscom_tx_ptr = nullptr;
    boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>> syscom_rx_ptr = nullptr;
    boost::shared_ptr<syscom_message_counter> syscom_message_counter_ptr = nullptr;

    std::string file_path = "";
    std::string decrypted_file_path = "";
    static boost::mutex mu;

public:

    explicit Worker(int _id, KeyRange _range, std::string _file_path, std::string _decrypted_path,
                    boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>> syscom_tx_ref,
                    boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>> syscom_rx_ref,
                    boost::shared_ptr<syscom_message_counter> counter_ptr)
            : id{_id},
              range(_range),
              syscom_tx_ptr{syscom_tx_ref},
              syscom_rx_ptr{syscom_rx_ref},
              syscom_message_counter_ptr{counter_ptr},
              file_path{_file_path},
              decrypted_file_path{_decrypted_path}
    { ; }

    std::vector<unsigned char> uint64ToBytes(uint64_t value) noexcept {
        std::vector<unsigned char> result(8, 0x00);
        result.push_back((value >> 56) & 0xFF);
        result.push_back((value >> 48) & 0xFF);
        result.push_back((value >> 40) & 0xFF);
        result.push_back((value >> 32) & 0xFF);
        result.push_back((value >> 24) & 0xFF);
        result.push_back((value >> 16) & 0xFF);
        result.push_back((value >>  8) & 0xFF);
        result.push_back((value) & 0xFF);
        return result;
    }

    std::string hashString(const std::string& str) noexcept {
        std::string result;
        CryptoPP::SHA1 sha1;
        CryptoPP::StringSource(str, true,
                               new CryptoPP::HashFilter(sha1, new CryptoPP::HexEncoder(
                                       new CryptoPP::StringSink(result), true)));
        return result;
    }

    void worker_process() {
        while (this->process){

            // check if there's any new syscom message... and process message if available
            SysComMessage sys_msg;
            while (syscom_rx_ptr->pop(sys_msg)) {
                switch(sys_msg.event){
                    case SysComMessage::Event::PING:{
                        SysComMessage syscom_msg{(*this->syscom_message_counter_ptr)++, this->id, SysComMessage::Event::CALLBACK, false, std::to_string(this->current_key), SysComMessage::Callback{sys_msg.id, sys_msg.event}};
                        this->syscom_tx_ptr->push(syscom_msg);
                    }break;

                    case SysComMessage::Event::WORKER_REARRANGE: {
                        assert(!this->work);

                        this->work = true;
                        // TODO: new key range to process...
                    }break;

                    case SysComMessage::Event::INTERRUPT:{
                        // Thread is about to finish it's existance...
                        this->stop();
                        this->finish();
                        SysComMessage syscom_msg{(*this->syscom_message_counter_ptr)++, this->id, SysComMessage::Event::CALLBACK, false, "interrupt data here...", SysComMessage::Callback{sys_msg.id, sys_msg.event}};
                        this->syscom_tx_ptr->push(syscom_msg);
                    }break;
                    default:
                        // TODO: send invalid message...
                        break;
                }
            }

            if (this->work) {
                if (this->current_key == this->range.end + 1) {
                    this->stop();

                    SysComMessage syscom_msg{(*this->syscom_message_counter_ptr)++, this->id, SysComMessage::Event::WORKER_DONE, false, "done"};
                    this->syscom_tx_ptr->push(syscom_msg);

                } else {

                    auto key_bytes = this->uint64ToBytes(this->current_key);

                    assert(key_bytes.size() == CryptoPP::AES::DEFAULT_KEYLENGTH);
                    byte* key = key_bytes.data();

                    byte iv[CryptoPP::AES::BLOCKSIZE];
                    memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE);

                    // TODO: make file load only once
                    std::ifstream file_stream(this->file_path, std::ios::binary);
                    std::stringstream read_stream;
                    read_stream << file_stream.rdbuf();
                    std::string read_stream_str = read_stream.str();

                    std::vector<std::string> file_lines;
                    boost::split(file_lines, read_stream_str, boost::is_any_of("\n"));

                    std::string sha1 = file_lines[0];
                    std::string ciphertext = "";
                    for (int i = 1; i < file_lines.size(); i++) {
                        ciphertext += file_lines[i];
                        if (i < file_lines.size() - 2)
                            ciphertext += '\n';
                    }

                    CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
                    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);

                    try {
                        std::string decryptedtext;
                        CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedtext));
                        stfDecryptor.Put(reinterpret_cast<const unsigned char *>(ciphertext.c_str()), ciphertext.size());
                        stfDecryptor.MessageEnd();

                        if (decryptedtext.back() == '\0')
                            decryptedtext.pop_back();

                        std::string decrypted_sha = this->hashString(decryptedtext);

                        if (decrypted_sha == sha1) {
                            std::cout << "[debug, Worker: " << this->id << "]: FOOUND for key: " << this->current_key << std::endl;

                            SysComMessage syscom_msg{(*this->syscom_message_counter_ptr)++, this->id, SysComMessage::Event::KEY_FOUND, false, std::to_string(this->current_key)};
                            this->syscom_tx_ptr->push(syscom_msg);
                            this->stop();

                            std::ofstream os(this->decrypted_file_path, std::ios::binary);
                            os << decryptedtext;
                            os.flush();
                            os.close();
                        }else{
//                            boost::unique_lock<boost::mutex> lock(Worker::mu);
//                            std::cout << "[debug, Worker: " << this->id << "]: non except -> key: " << this->current_key << ", decrypted sha: " << decrypted_sha << " sha: " << sha1 << std::endl;
                        }

                    } catch (const CryptoPP::InvalidCiphertext& e) {
                        // ...
//                        boost::unique_lock<boost::mutex> lock(Worker::mu);
//                        std::cout << "[debug, Worker: " << this->id << "]: exception for key: " << this->current_key << std::endl;
                    }

                    this->current_key += 1;

                }
            }

            //boost::this_thread::sleep_for(boost::chrono::nanoseconds(5));
        }
    }

    void start(){
        if (!this->process) {
            this->process = true;

            if (!this->work)
                this->work = true;

            this->current_key = this->range.begin;
            this->worker_process();
        } else
            throw std::runtime_error{"Worker already started! 0x0004"};
    }

    void stop(){
        this->work = false;
    }

    void finish(){
        this->process = false;
    }

};

boost::mutex Worker::mu = {};

auto calculate_range = [](uint64_t absolute_key_from, uint64_t absolute_key_to, int size)
        -> boost::container::vector<std::pair<uint64_t, uint64_t>> {

    assert(absolute_key_from < absolute_key_to);
    uint64_t range = absolute_key_to - absolute_key_from;
    double ratio = static_cast<double>(range) / static_cast<double>(size);
    boost::container::vector<std::pair<uint64_t, uint64_t>> ranges{};

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


auto constexpr receive_thread_loop_delay    = 100000u;         /* ns -> 0.1ms */
auto constexpr process_message_loop_delay   = 10000000u;       /* ns  -> 10ms */

struct Options{
    uint64_t absolute_key_from   = 0;
    uint64_t absolute_key_to     = 0;
};

bool isAlive = true;

std::string encrypted_file = "";
std::string decrypt_file = "decrypted.txt";

int main(int argc, const char* argv[]) {
    std::cout << std::boolalpha;
    std::cout.sync_with_stdio(true);

    Options options;

    po::options_description desc("DDS Options");
    desc.add_options()
            ("help", "produce help MpiMessage")
            ("from", po::value<uint64_t>(), "set key range BEGIN value")
            ("to", po::value<uint64_t>(), "set key range END value")
            ("encrypted", po::value<std::string>(), "encrypted file path")
            ("decrypt", po::value<std::string>(), "decrypted file path")
            ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    if (vm.count("from"))
        options.absolute_key_from = vm["from"].as<uint64_t>();
    else {
        std::cerr << "Set min range!" << std::endl;
        return 1;
    }

    if (vm.count("to"))
        options.absolute_key_to = vm["to"].as<uint64_t>();
    else {
        std::cerr << "Set max range!" << std::endl;
        return 1;
    }

    if (vm.count("encrypted")) {
        ::encrypted_file = vm["encrypted"].as<std::string>();
    } else {
        std::cerr << "Encrypted file path missing!" << std::endl;
        return 1;
    }

    if (vm.count("decrypt")) {
        ::decrypt_file = vm["decrypt"].as<std::string>();
    } else {
        std::cout << "Decrypt file path not set... Using default: " << ::decrypt_file << std::endl;
    }

    assert(options.absolute_key_from >= 0 && options.absolute_key_to > 0);
    assert(options.absolute_key_from < options.absolute_key_to);

    mpi::environment env(mpi::threading::multiple, true);
    mpi::communicator world;

    Master master;
    Slave slave;

    if (world.rank() == 0) {

        std::cout << "[debug: " << world.rank() << "] Initial key range: [" << options.absolute_key_from << ", " << options.absolute_key_to << "]" << std::endl;

        boost::container::vector<std::pair<uint64_t, uint64_t>> ranges =
                boost::move(calculate_range(options.absolute_key_from, options.absolute_key_to, (world.size() - 1)));


        std::cout << "[debug: " << world.rank() << "] Calculated ranges: " << std::endl;
        for (const auto& e : ranges)
            std::cout << "[debug: " << world.rank() << "] \t[" << e.first << ", " << e.second << "]" << std::endl;

        boost::lockfree::spsc_queue<MpiMessage> receive_queue(128);
        boost::lockfree::spsc_queue<MpiMessage> send_queue(128);

        auto thread_pool = boost::thread::hardware_concurrency();
        auto used_threads = 2; // MPI_thread, Process_thread

        boost::atomic_uint32_t mpi_message_id{0};
        boost::container::vector<int> ready_node;

        boost::container::map<rank_type, boost::container::map<int, uint64_t>> nodemap; // node - workers (progress)
        boost::container::map<rank_type, boost::container::vector<std::pair<uint64_t, uint64_t>>> nodeinfo;

        boost::shared_mutex watchdog_mutex;
        boost::container::map<uint32_t, std::pair<rank_type, int>> watchdog_need_callback{}; // <message_id, <node, kick_number>>

        uint64_t match_found = 0;

        std::cout << "[debug: " << world.rank() << "] Thread pool: " << thread_pool << std::endl;
        std::cout << "[debug: " << world.rank() << "] Master (MPI) main thread rank: " << boost::this_thread::get_id() << std::endl;

        auto process_thread_implementation = [&]{
            std::cout << "[debug: " << world.rank() << "] Process thread rank: " <<  boost::this_thread::get_id() <<  " has been triggered!" << std::endl;

            uint64_t received = 0l;
            while (isAlive){

                boost::function<void(void)> pinger_implementation{[&](void) -> void {
                    boost::unique_lock<boost::shared_mutex> lock(watchdog_mutex);

                    if (isAlive) {
                        for (int i = 1; i < world.size(); i++) {
                            if (std::find(ready_node.begin(), ready_node.end(), i) != ready_node.end()) {
                                auto it = std::find_if(watchdog_need_callback.begin(), watchdog_need_callback.end(),
                                                       [&](const auto &e) {
                                                           return (e.second).first == i;
                                                       });
                                if (it == watchdog_need_callback.end()) {
                                    auto msg_id = mpi_message_id++;
                                    send_queue.push({msg_id, i, world.rank(), MpiMessage::Event::PING, true, "?"});

                                    if (watchdog_need_callback.size() > 0) {
                                        if (watchdog_need_callback.find(msg_id) == watchdog_need_callback.end())
                                            watchdog_need_callback[msg_id] = std::make_pair(i, 0);
                                        else
                                            throw std::runtime_error{"Message already registered in Watchdog!"};
                                    } else
                                        watchdog_need_callback[msg_id] = std::make_pair(i, 0);
                                } else {

                                    boost::container::vector<rank_type> lost_slaves;

                                    for (auto& t : watchdog_need_callback) {
                                        if ((t.second.second) == 1)
                                            std::cout << "<<Watchdog: Still waiting for message: \t{" << t.first << ", " << t.second.first << ", " << (t.second.second)++ << "}" << std::endl;
                                        else if ((t.second.second) > 2){
                                            std::cout << "[debug: " << world.rank() << "] <<Wachdog: Slave: " << t.second.first  << " is dead, needs to be handled!" << std::endl;
                                            lost_slaves.push_back(t.second.first);
                                        }
                                    }

                                    if (lost_slaves.size() > 0) {
                                        std::cout << "[debug: " << world.rank() << "] <<Wachdog: Handeling - there are: " << lost_slaves.size() << " slaves dead" << std::endl;
                                        for (auto i : lost_slaves) {

                                            // TODO: abstract - code replication!
                                            // find first registered message for this slave
                                            auto wd_it = std::find_if(watchdog_need_callback.begin(), watchdog_need_callback.end(),
                                                                      [i](const std::pair<uint32_t, std::pair<rank_type, int>>& element){
                                                                          return element.second.first == i;
                                                                      }
                                            );

                                            while (wd_it != watchdog_need_callback.end()){
                                                watchdog_need_callback.erase(wd_it);

                                                // find next registered message for slave
                                                wd_it = std::find_if(watchdog_need_callback.begin(), watchdog_need_callback.end(),
                                                                     [i](const std::pair<uint32_t, std::pair<rank_type, int>>& element){
                                                                         return element.second.first == i;
                                                                     }
                                                );
                                            }
                                        }

                                        auto it = std::find(ready_node.begin(), ready_node.end(), i);
                                        assert(it != ready_node.end());

                                        ready_node.erase(std::remove(ready_node.begin(), ready_node.end(), i));
                                        assert(std::find(ready_node.begin(), ready_node.end(), i) == ready_node.end());
                                        ready_node.shrink_to_fit();

                                        std::cout << "[debug: " << world.rank() << "] <<Wachdog: Dead Slave: " << i << " handeled." << std::endl;
                                    }
                                }
                            } else {
                                std::cout << "[debug: " << world.rank() << "] <<Watchdog: Skipping ping for: " << i << " - (not ready || dead)" << std::endl;

                                // check if there are any registered messages for this slave - this can happen if the slave has done.

                                // TODO: abstract - code replication!
                                auto wd_it = std::find_if(watchdog_need_callback.begin(), watchdog_need_callback.end(),
                                                          [i](const std::pair<uint32_t, std::pair<rank_type, int>>& element){
                                                              return element.second.first == i;
                                                          }
                                );

                                if (wd_it != watchdog_need_callback.end()) {
                                    std::cout << "[debug: " << world.rank() << "] <<Watchdog: Oh! there are some registered messages for not ready slave.." << std::endl;
                                    while (wd_it != watchdog_need_callback.end()) {
                                        watchdog_need_callback.erase(wd_it);

                                        wd_it = std::find_if(watchdog_need_callback.begin(),
                                                             watchdog_need_callback.end(),
                                                             [i](const std::pair<uint32_t, std::pair<rank_type, int>> &element) {
                                                                 return element.second.first == i;
                                                             }
                                        );
                                    }
                                }

                            }
                        }
                    }
                }};

                CallBackTimer pinger;
                pinger.start(boost::chrono::milliseconds(3000), pinger_implementation);


                auto message_processing = [&] {
                    MpiMessage msg;
                    while (receive_queue.pop(msg)) {
                        std::cout << "[debug: " << world.rank() << "]: Processing... (received " << ++received << ") MpiMessage: " << std::endl;
                        std::cout << "      {" << "id: " << msg.id
                                  << ", sender: " << msg.sender
                                  << ", receiver: " << msg.receiver
                                  << ", event: " << std::to_string(static_cast<int>(msg.event))
                                  << ", data: " << msg.data
                                  << ", need_response: " << msg.need_respond
                                  << "}" << std::endl;

                        switch (msg.event){
                            case MpiMessage::Event::CALLBACK:{
                                boost::unique_lock<boost::shared_mutex> lock(watchdog_mutex);

                                MpiMessage::Callback callback_msg_id;
                                if (msg.respond_to)
                                    callback_msg_id = *(msg.respond_to);
                                else
                                    throw std::runtime_error{"Cannot get message_to optional!"};

                                std::cout << "[debug: " << world.rank() << "]: Received CALLBACK message with id: " << msg.id << " for {msg_id: " << callback_msg_id.message_id << ", event: " << std::to_string(
                                        static_cast<int>(callback_msg_id.event)) << "} form rank: " << msg.sender << std::endl;

                                auto watchdog_element_iterator = watchdog_need_callback.find(callback_msg_id.message_id);
                                if (watchdog_element_iterator != watchdog_need_callback.end()) {
                                    watchdog_need_callback.erase(watchdog_element_iterator);
                                    std::cout << "[debug: " << world.rank() << "]: Watchdog unregistered message: {" << callback_msg_id.message_id << ", event: " << std::to_string(
                                            static_cast<int>(callback_msg_id.event)) << "}" << std::endl;
                                }else{
                                    std::cout << "[error: " << world.rank() << "]: Watchdog could not find message: " << callback_msg_id.message_id << std::endl;
                                    // TODO: maybe because of slave is already offline...
                                }

                                // TODO: Process callback...
                                switch (callback_msg_id.event){
                                    case MpiMessage::Event::INIT:{
                                        std::cout << "[info: " << world.rank() << "]: - - - - FINISHED INIT FOR node: " << msg.sender << std::endl;

                                        if (nodemap.find(msg.sender) != nodemap.end())
                                            throw std::runtime_error{"Error mapping node with workers..."};

                                        int workers = std::atoi(msg.data.c_str());

                                        std::cout << "[debug: " << world.rank() << "]: Setting number of workers for node: " << msg.sender << " on: " << std::to_string(workers) << std::endl;
                                        std::cout << "[debug: " << world.rank() << "]: Calculating Workers ranges in Master for: " << ranges[msg.sender - 1].first << " - " << ranges[msg.sender - 1].second  << std::endl;

                                        boost::container::vector<std::pair<uint64_t, uint64_t>> worker_ranges =
                                                boost::move(calculate_range(ranges[msg.sender - 1].first, ranges[msg.sender - 1].second, workers));

                                        for (const auto& range : worker_ranges)
                                            std::cout << "[debug: " << world.rank() << "] \t[" << range.first << ", " << range.second << "]" << std::endl;

                                        nodeinfo[msg.sender] = boost::move(worker_ranges);

                                        boost::container::map<int, uint64_t> temp{};
                                        for (int i = 0; i < workers; i++){
                                            temp[i] = 0;
                                        }

                                        assert(static_cast<int>(temp.size()) == workers);

                                        nodemap[msg.sender] = boost::move(temp);
                                        ready_node.push_back(msg.sender);

                                    }break;

                                    case MpiMessage::Event::PING:{
                                        std::cout << "[info: " << world.rank() << "]: - - - - PING FROM node: " << msg.sender << std::endl;

                                        boost::container::vector<std::string> report_strings{};
                                        boost::split(report_strings, msg.data, ::isspace);

                                        assert(report_strings.size() == nodemap[msg.sender].size());

                                        boost::container::map<std::string, std::string> report_mapped{};

                                        ///////////////
                                        for (const std::string& s : report_strings) {
                                            std::string::size_type key_pos = 0;
                                            std::string::size_type key_end;
                                            std::string::size_type val_pos;
                                            std::string::size_type val_end;

                                            while ((key_end = s.find(':', key_pos)) != std::string::npos) {
                                                if ((val_pos = s.find_first_not_of(":", key_end)) == std::string::npos)
                                                    break;

                                                val_end = s.find('\n', val_pos);
                                                report_mapped.emplace(s.substr(key_pos, key_end - key_pos), s.substr(val_pos, val_end - val_pos));

                                                key_pos = val_end;
                                                if (key_pos != std::string::npos)
                                                    ++key_pos;
                                            }
                                        }
                                        ///////////////

                                        assert(report_mapped.size() == nodemap[msg.sender].size());

                                        boost::container::map<int, uint64_t>& pingmap = nodemap[msg.sender];

                                        for (const auto& e : report_mapped){
                                            int index = boost::lexical_cast<int>(e.first);
                                            uint64_t value = boost::lexical_cast<uint64_t>(e.second);

                                            if (pingmap.find(index) == pingmap.end())
                                                throw std::runtime_error{"No worker thread in pingmap!"};

                                            std::cout << "[info: " << world.rank() << "]: Storing report fot: {node:" << msg.sender << ", worker: " << std::to_string(index) << ", value: " << value << "}" << std::endl;

                                            pingmap[index] = value;
                                        }

                                        assert(pingmap.size() == nodemap[msg.sender].size());

                                    }break;
                                    default:{
                                        // invalide message...
                                    }break;
                                }

                            }break;

                            case MpiMessage::Event::FOUND:{

                                uint64_t match = boost::lexical_cast<uint64_t>(msg.data);
                                match_found = match;

                                std::cout << "[info: " << world.rank() << "] ~~~ Master received KEY FOUND form Slave: " << msg.sender << ", key: " <<  match_found << " ~~~" << std::endl;
                                std::cout << "[info: " << world.rank() << "] Master sends KILL message to all slaves" << std::endl;

                                for (int slave : ready_node)
                                    send_queue.push({mpi_message_id++, slave, world.rank(), MpiMessage::Event::KILL, false, "kill"});

                            }break;

                            case MpiMessage::Event::SLAVE_DONE:{
                                boost::unique_lock<boost::shared_mutex> lock(watchdog_mutex); //, boost::defer_lock);

                                std::cout << "[info: " << world.rank() << "]: Master received SLAVE_DONE from: " << msg.sender << std::endl;

                                std::cout << "[info: " << world.rank() << "] Master setting slave: " << msg.sender << " offline"<< std::endl;

                                ready_node.erase(std::remove(ready_node.begin(), ready_node.end(), msg.sender));
                                assert(std::find(ready_node.begin(), ready_node.end(), msg.sender) == ready_node.end());
                                ready_node.shrink_to_fit();

                                if (ready_node.size() > 0)
                                    std::cout << "[info: " << world.rank() << "] There are: " << ready_node.size() << " slave nodes left" << std::endl;
                                else {
                                    std::cout << "[info: " << world.rank() << "] There are no running slaves left - Master is going down..." << std::endl;
                                    isAlive = false;
                                }

                            }break;

                            case MpiMessage::Event::SLAVE_WORKER_DONE:{
                                boost::container::vector<std::string> range_str;
                                boost::split(range_str, msg.data, boost::is_any_of(":"));
                                assert(range_str.size() == 2);

                                uint64_t range_begin = boost::lexical_cast<uint64_t>(range_str[0]);
                                uint64_t range_end = boost::lexical_cast<uint64_t>(range_str[1]);

                                // TODO: handle slave worker done...
                                std::cout << "[info: " << world.rank() << "] Slave's: " << msg.sender << " finished range: [" << range_begin << ", " << range_end << "]" << std::endl;

                            }break;

                            default:{
                                // TODO: invalide message!
                            }break;
                        }

                    }
                    boost::this_thread::sleep_for(boost::chrono::nanoseconds(process_message_loop_delay));
                };

                message_processing();

            }
        };

        boost::thread process_thread(process_thread_implementation);

        boost::this_thread::sleep_for(boost::chrono::seconds(2));
        std::cout << "[debug: " << world.rank() << "] " << "----- Init ping pong squence: STARTS -----" << std::endl;
        for (int i = 1; i < world.size(); i++) {
            boost::unique_lock<boost::shared_mutex> lock(watchdog_mutex);

            std::cout << "[debug: " << world.rank() << "] " << "Sending init data to: " << i << std::endl;

            std::stringstream report;
            report << ranges[i - 1].first << ":" << ranges[i - 1].second;
            std::string report_str = report.str();

            auto msg_id = mpi_message_id++;
            send_queue.push({msg_id, i, world.rank(), MpiMessage::Event::INIT, true, report_str});

            // TODO: watchdog register mechanizm
            if (watchdog_need_callback.size() > 0){
                if (watchdog_need_callback.find(msg_id) == watchdog_need_callback.end())
                    watchdog_need_callback[msg_id] = std::make_pair(i, 0);
                else
                    throw std::runtime_error{"Message already registered in Watchdog!"};
            }else
                watchdog_need_callback[msg_id] = std::make_pair(i, 0);

            std::cout << "[debug: " << world.rank() << "] " << "Watchdog registered message with id: " << msg_id << " for rank: " << i << std::endl;
        }
        std::cout << "[debug: " << world.rank() << "] " << "----- Init ping pong squence: ENDS -----" << std::endl;

        while (isAlive) {
            boost::optional<mpi::status> stat = world.iprobe(mpi::any_source, mpi::any_tag);
            if (stat){
                std::cout << "[debug: " << world.rank() << "] Receive thread has probed a MpiMessage..."<< std::endl;

                MpiMessage received_message;
                world.recv((*stat).source(), (*stat).tag(), received_message);
                receive_queue.push(boost::move(received_message));

                std::cout << "[debug: " << world.rank() << "] Receive thread has received MpiMessage\n    data:" << received_message.data << std::endl;
            }

            if (!send_queue.empty()){
                MpiMessage msg;
                while (send_queue.pop(msg)){
                    std::cout << "[debug: " << world.rank() << "] " << "Message {target: " << msg.receiver << ", data: " << msg.data << "} is about to send" << std::endl;
                    world.send(msg.receiver, 0, msg);
                    std::cout << "[debug: " << world.rank() << "] " << "Message sent." << std::endl;
                }
            }
            boost::this_thread::sleep_for(boost::chrono::nanoseconds(receive_thread_loop_delay));
        }

        process_thread.join();

        std::cout << "[info: " << world.rank() << "] Master is down. " << std::endl;
        exit(EXIT_SUCCESS);

    } else {

        boost::lockfree::spsc_queue<MpiMessage> receive_queue(128);
        boost::lockfree::spsc_queue<MpiMessage> send_queue(128);

        auto thread_pool = boost::thread::hardware_concurrency();
        auto used_threads = 2; // MPI_thread, Process_thread

        uint32_t mpi_message_id = 0;
        boost::shared_ptr<syscom_message_counter> syscom_message_counter_ptr = boost::make_shared<syscom_message_counter>();

        std::cout << "[debug: " << world.rank() << "] Thread pool: " << thread_pool << std::endl;
        std::cout << "[debug: " << world.rank() << "] Master (MPI) main thread rank: " << boost::this_thread::get_id() << std::endl;

        auto process_thread_implementation = [&]{
            std::cout << "[debug: " << world.rank() << "] Process thread rank: " <<  boost::this_thread::get_id() <<  " has been triggered!" << std::endl;

            auto avaiable_threads = thread_pool - used_threads;

            boost::container::vector<boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>>> syscom_thread_tx{};
            boost::container::vector<boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>>> syscom_thread_rx{};

            boost::container::vector<boost::shared_ptr<boost::thread>> thread_array{};

            boost::container::vector<boost::shared_ptr<Worker>> worker_pointers{};

            boost::container::vector<std::pair<int, bool>> worker_alive{};
            boost::container::vector<std::pair<int, bool>> worker_running{};

            boost::container::vector<std::pair<uint64_t, uint64_t>> worker_ranges{};
            boost::container::vector<std::pair<uint64_t, uint64_t>> new_key_ranges{}; // ???

            bool isInit = false;

            // TODO: abstract PING handler!
            MpiMessage ping_msg;
            boost::container::vector<uint64_t> ping_info(avaiable_threads); // <worker, key_value>
            int collected = 0;

            auto init_worker_threads = [&]{
                for (int i = 0; i < avaiable_threads; i++){
                    std::cout << "[debug: " << world.rank() << "] Preparing syscom for Worker for thread: " << i << std::endl;

                    boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>> sc_tx = boost::make_shared<boost::lockfree::spsc_queue<SysComMessage>>(128);
                    syscom_thread_tx.push_back(sc_tx);

                    boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>> sc_rx = boost::make_shared<boost::lockfree::spsc_queue<SysComMessage>>(128);
                    syscom_thread_rx.push_back(sc_rx);

                    std::cout << "[debug: " << world.rank() << "] Preparing Worker for thread: " << i << std::endl;

                    auto thread_task = [i, sc_rx, sc_tx, syscom_message_counter_ptr, &worker_pointers, &worker_ranges]{
                        boost::shared_ptr<Worker> worker_ptr = boost::make_shared<Worker>(i, Worker::KeyRange{worker_ranges[i].first, worker_ranges[i].second}, ::encrypted_file, ::decrypt_file, sc_rx, sc_tx, syscom_message_counter_ptr);
                        worker_pointers.push_back(worker_ptr);
                        worker_ptr->start();
                    };
                    boost::shared_ptr<boost::thread> worker_thread_ptr = boost::make_shared<boost::thread>(thread_task);
                    thread_array.push_back(worker_thread_ptr);

                    worker_alive.emplace_back(i, true);
                    worker_running.emplace_back(i, true);
                }
                assert(worker_alive.size() > 0 && worker_running.size() > 0 && worker_alive.size() == worker_running.size());
            };

            auto process_syscom_message = [&](SysComMessage& sys_msg) {
                switch (sys_msg.event) {
                    case SysComMessage::Event::PING: {
                        std::cout << "[debug: " << world.rank() << "] Syscom: PING EVENT: "
                                  << "{rank:" << sys_msg.rank << ", data: " << sys_msg.data << "}"
                                  << std::endl;

                    }break;

                        //When there's a callback from worker e.g. ping callback
                    case SysComMessage::Event::CALLBACK: {
                        SysComMessage::Callback cb = *(sys_msg.respond_to);

                        std::cout << "[debug: " << world.rank() << "] Syscom: CALLBACK EVENT: "
                                  << "{rank:" << sys_msg.rank << " respons to: {message_id: "
                                  << cb.message_id << ", type: " << std::to_string(
                                static_cast<int>(cb.event)) << "}" << ", data: " << sys_msg.data << "}"
                                  << std::endl;

                        switch (cb.event) {
                            case SysComMessage::Event::PING: {
                                std::cout << "[debug: " << world.rank()
                                          << "] Syscom: Processing CALLBACK event for PING: "
                                          << sys_msg.rank << "..." << std::endl;

                                // TODO: abstract PING registration logic to class
                                if (collected == avaiable_threads - 1) {
                                    if (ping_info[sys_msg.rank] == 0)
                                        ping_info[sys_msg.rank] = static_cast<uint64_t>(std::atoll(
                                                sys_msg.data.c_str()));
                                    else
                                        throw std::runtime_error{"Ping info contains element for this key already! 0x002"};

                                    std::stringstream report;
                                    int i = 0;
                                    for (auto &element : ping_info) {
                                        report << i << ":" << element << ' ';
                                        ping_info[i] = 0;
                                        i++;
                                    }
                                    collected = 0;
                                    std::string report_str = report.str();
                                    assert(std::isspace(report_str.back()));
                                    report_str.erase(report_str.begin() + report_str.length() - 1);

                                    std::cout << "[debug: " << world.rank()
                                              << "] Syscom: Processing CALLBACK event for PING: "
                                              << sys_msg.rank << ", sending data: {" << report_str << "}"
                                              << std::endl;

                                    send_queue.push({mpi_message_id++, ping_msg.sender, world.rank(),
                                                     MpiMessage::Event::CALLBACK,
                                                     false, report_str,
                                                     MpiMessage::Callback{ping_msg.id, ping_msg.event}});

                                } else {
                                    if (ping_info[sys_msg.rank] == 0) {
                                        ping_info[sys_msg.rank] = static_cast<uint64_t>(std::atoll(sys_msg.data.c_str()));
                                        collected += 1;
                                    } else
                                        throw std::runtime_error{
                                                "Ping info contains element for this key already! 0x001"};
                                }

                            }break;

                            case SysComMessage::Event::INTERRUPT: {
                                std::cout << "[debug: " << world.rank() << "] Syscom: Processing CALLBACK event for INTERRUPT form Worker: " << sys_msg.rank << std::endl;
                                std::cout << "[debug: " << world.rank() << "] Syscom: Worker: " << sys_msg.rank << " is turned off!" << std::endl;

                                auto it = std::find(worker_alive.begin(), worker_alive.end(), std::pair<int, bool>(sys_msg.rank, true));
                                if (it == worker_alive.end())
                                    throw std::runtime_error{"0x003"};
                                *it = std::make_pair(sys_msg.rank, false);

                                int still_alive = 0;
                                for (const auto& element : worker_alive){
                                    if (element.second)
                                        still_alive++;
                                }

                                if (still_alive > 0)
                                    std::cout << "[debug: " << world.rank() << "] Syscom: There are still: " << still_alive << " workers ALIVE. " << std::endl;
                                else {
                                    std::cout << "[debug: " << world.rank() << "] Syscom: There are NO workers up! Slave node: " << world.rank()  << " is about to close - Sending message to Master" << std::endl;

                                    int i = 0;
                                    for (auto& thread_ptr : thread_array) {
                                        worker_pointers[i++].reset();
                                        thread_ptr->join();
                                        thread_ptr.reset();
                                    }

                                    send_queue.push({mpi_message_id++, 0, world.rank(), MpiMessage::Event::SLAVE_DONE, false, "slave_done"});
                                }
                            }break;

                            default: {
                                std::cout << "[error: " << world.rank() << "] Unknow SYSCOM CALLBACK message!" << std::endl;
                            }break;
                        }

                    }break;

                    case SysComMessage::Event::WORKER_DONE: {
                        std::cout << "[debug: " << world.rank() << "] Syscom: Processing *** WORKER DONE for Worker thread: " << sys_msg.rank << " ***"<< std::endl;

                        auto it = std::find_if(worker_running.begin(), worker_running.end(), [&](const auto& e) {
                            return e.first == sys_msg.rank && e.second == true;
                        });
                        if (it == worker_running.end())
                            throw std::runtime_error{"0x005"};

                        *it = std::make_pair(sys_msg.rank, false);

                        int still_running = 0;
                        for (const auto& element : worker_running){
                            if (element.second)
                                still_running++;
                        }

                        std::stringstream ss;
                        ss << worker_ranges[sys_msg.rank].first << ":" << worker_ranges[sys_msg.rank].second;
                        send_queue.push({mpi_message_id++, 0, world.rank(), MpiMessage::Event::SLAVE_WORKER_DONE, false, ss.str()});

                        if (new_key_ranges.size() > 0){
                            std::cout << "[debug: " << world.rank() << "] Syscom: Assigning a new key range to worker..." << std::endl;
                            // TODO: if the worker has finished it's own job, assigne a new key range to process if there is one...
                        }else {
                            // wait until last worker is processing... if the last has finished and there's no new key range to process kill workers
                            if (still_running == 0) {
                                std::cout << "[debug: " << world.rank()
                                          << "] Syscom: There is no need for handle new key range & all workers has finished. Sending INTERRUPT to ALL workers."
                                          << std::endl;

                                int i = 0;
                                for (auto& comm : syscom_thread_tx){
                                    SysComMessage syscom_msg{(*syscom_message_counter_ptr)++, i++, SysComMessage::Event::INTERRUPT, true, "interrupt"};
                                    comm->push(syscom_msg);
                                }

                            }else{
                                std::cout << "[debug: " << world.rank() << "] Syscom: There are still: " << still_running << " workers working." << std::endl;
                            }
                        }
                    }break;

                    case SysComMessage::Event::KEY_FOUND: {

                        uint64_t match = boost::lexical_cast<uint64_t>(sys_msg.data);

                        std::cout << "[error: " << world.rank() << "] ~~~ Worker: " << sys_msg.rank << ", KEY FOUND: " << match << " ~~~" << std::endl;

                        send_queue.push({mpi_message_id++, 0, world.rank(), MpiMessage::Event::FOUND, false, sys_msg.data});

                    }break;

                    default: {
                        std::cout << "[error: " << world.rank() << "] Unknow SYSCOM message!" << std::endl;
                    }break;
                }
            };

            auto message_processing = [&] {
                uint64_t received = 0;

                while (isAlive) {

                    MpiMessage msg;

                    while (receive_queue.pop(msg)) {
                        std::cout << "[debug: " << world.rank() << "]: Processing... (received " << ++received
                                  << ") MpiMessage: \n";
                        std::cout << "      {" << "sender: " << msg.sender << ", data: " << msg.data << "}"
                                  << std::endl;

                        // TODO: message processing logic class
                        switch (msg.event) {
                            case MpiMessage::Event::PING: {
                                std::cout << "[debug: " << world.rank()
                                          << "]: Processing PING request, sending PING request to workers SYSCOM."
                                          << std::endl;
                                int i = 0;
                                for (auto &comm : syscom_thread_tx) {
                                    SysComMessage syscom_msg{(*syscom_message_counter_ptr)++, i++,
                                                             SysComMessage::Event::PING, true, "???"};
                                    comm->push(syscom_msg);
                                }

                                ping_msg = msg;

                            }break;

                            case MpiMessage::Event::INIT: {
                                if (!isInit) {
                                    std::cout << "[debug: " << world.rank() << "]: Init message processing..." << std::endl;
                                    isInit = true;
                                    boost::container::vector<std::string> range_str;
                                    boost::split(range_str, msg.data, boost::is_any_of(":"));
                                    assert(range_str.size() == 2);

                                    uint64_t range_begin = boost::lexical_cast<uint64_t>(range_str[0]);
                                    uint64_t range_end = boost::lexical_cast<uint64_t>(range_str[1]);

                                    std::cout << "[debug: " << world.rank() << "]: Slave: " << world.size() << " Received range: [" << range_begin << ", " << range_end << "]" << std::endl;

                                    worker_ranges = boost::move(calculate_range(range_begin, range_end, avaiable_threads));

                                    std::cout << "[debug: " << world.rank() << "]: Calculating ranges for workers in SLAVE." << std::endl;

                                    for (const auto& e : worker_ranges)
                                        std::cout << "[debug: " << world.rank() << "]: \t[" << e.first << ", " << e.second << "]" << std::endl;

                                    init_worker_threads();

                                    std::stringstream ss;
                                    ss << avaiable_threads << std::endl;
                                    data_type data{ss.str()};
                                    send_queue.push({mpi_message_id++, msg.sender, world.rank(), MpiMessage::Event::CALLBACK,
                                                     false, data, MpiMessage::Callback{msg.id, msg.event}});
                                } else {
                                    // TODO: send invalide operation...
                                }
                            }break;

                            case MpiMessage::Event::KILL: {
                                std::cout << "[debug: " << world.rank() << "]: Slave: " << world.size() << " received MPI KILL message - stopping slave" << std::endl;

                                int i = 0;
                                for (auto& comm : syscom_thread_tx){
                                    SysComMessage syscom_msg{(*syscom_message_counter_ptr)++, i++, SysComMessage::Event::INTERRUPT, true, "interrupt"};
                                    comm->push(syscom_msg);
                                }

                            }break;

                            case MpiMessage::Event::SLAVE_REARRANGE:{
                                /* This message will be required for future DDS feature */
                            }break;

                            default: {
                                // TODO: send invalide operation...
                            }
                                break;
                        }
                    }

                    for (auto& comm : syscom_thread_rx) {
                        SysComMessage sys_msg_;

                        if (comm->pop(sys_msg_))
                            process_syscom_message(sys_msg_);
                    }

                    boost::this_thread::sleep_for(boost::chrono::nanoseconds(process_message_loop_delay));
                }

            };

            message_processing();
        };

        boost::thread process_thread(process_thread_implementation);
        process_thread.detach();

        while (isAlive) {
            boost::optional<mpi::status> stat = world.iprobe(mpi::any_source, mpi::any_tag);
            if (stat){
                std::cout << "[debug: " << world.rank() << "] Receive thread has probed a MpiMessage..."<< std::endl;

                MpiMessage received_message;
                world.recv((*stat).source(), (*stat).tag(), received_message);
                receive_queue.push(boost::move(received_message));

                std::cout << "[debug: " << world.rank() << "] Receive thread has received MpiMessage\n    data:" << received_message.data << std::endl;
            }

            if (!send_queue.empty()){
                MpiMessage msg;
                while (send_queue.pop(msg)){
                    std::cout << "[debug: " << world.rank() << "] " << "Message {target: " << msg.receiver << ", data: " << msg.data << "} is about to send" << std::endl;
                    world.send(msg.receiver, 0, msg);
                    std::cout << "[debug: " << world.rank() << "] " << "Message sent." << std::endl;
                }
            }
            boost::this_thread::sleep_for(boost::chrono::nanoseconds(receive_thread_loop_delay));
        }

        std::cout << "[debug: " << world.rank() << "] Slave: " << world.rank() << " clean up." << std::endl;
        std::cout << "[debug: " << world.rank() << "] *** Slave: " << world.rank() << " has FINISHED! ***" << std::endl;
    }

    return EXIT_SUCCESS;
}
