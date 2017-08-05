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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Gateway {
    constexpr static int max_operation_duration = 1000; //ms

    static boost::optional<std::string> _send_and_receive(boost::shared_ptr<mpi::communicator> world, const int rank, const int tag, const std::string &msg){
        std::string data{};
        mpi::request reqs[2];
        reqs[0] = world->isend(rank, tag, msg);
        reqs[1] = world->irecv(rank, tag, data);
        std::size_t time_duration = 0;

        while (true) {
            boost::chrono::steady_clock::time_point begin = boost::chrono::steady_clock::now();

            if (reqs[0].test() && reqs[1].test())
                return boost::optional<std::string>(data);

            //sleep
            boost::this_thread::sleep_for(boost::chrono::nanoseconds(100)); //1000ns = 1ms

            boost::chrono::steady_clock::time_point end = boost::chrono::steady_clock::now();
            time_duration += boost::chrono::duration_cast<boost::chrono::milliseconds>(end - begin).count();

            if (time_duration >= max_operation_duration)
                break;
        }

        return {};
    }


public:

    static boost::optional<std::string> send_and_receive(boost::shared_ptr<mpi::communicator> world, const int rank, const int tag, const std::string &msg){
        boost::mutex m;
        boost::condition_variable cv;
        boost::optional<std::string> ret;

        boost::thread t([&m, &cv, &ret, &world, &rank, &tag, &msg]() {
            ret = Gateway::_send_and_receive(world, rank, tag, msg);
            cv.notify_one();
        });
        t.detach();

        {
            boost::unique_lock<boost::mutex> l(m);
            if (cv.wait_for(l, boost::chrono::milliseconds(
                    max_operation_duration + static_cast<std::size_t>(0.2 * max_operation_duration))) ==
                boost::cv_status::timeout)
                throw std::runtime_error("Timeout: " + rank);
        }
        return ret;
    }

};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int main(int argc, const char* argv[]) {
    boost::shared_ptr<mpi::environment> env = boost::make_shared<mpi::environment>(mpi::threading::multiple, true);
    boost::shared_ptr<mpi::communicator> world = boost::make_shared<mpi::communicator>();

    if (world->rank() == 0){
        for (int i = 1; i < world->size(); i++) {
            try {
                boost::optional<std::string> response = Gateway::send_and_receive(world, i, 0, "hello");
                if (response) {
                    std::cout << "[" << world->rank() << "]: Master received: " << *response << std::endl;
                } else {
                    std::cerr << "[" << world->rank() << "]: Master did NOT received exception, but option is empty!" << std::endl;
                }
            } catch (const std::runtime_error & e) {
                std::cerr << "[" << world->rank() << "]: Master exception: " << e.what() << std::endl;
            }
        }
    } else {
        std::string msg;
        world->recv(0, 0, msg);
        std::cout << "[" << world->rank() << "]: Slave received: " << msg << std::endl;
        std::string response = std::to_string(world->rank());
        //boost::this_thread::sleep_for(boost::chrono::seconds(4));
        world->send(0, 0, response);
    }
}

//int main(int argc, const char* argv[]) {
//    std::cout << std::boolalpha;
//    std::cout.sync_with_stdio(true);
//
//    Options options;
//
//    po::options_description desc("DDS Options");
//    desc.add_options()
//            ("help", "produce help MpiMessage")
//            ("from", po::value<uint64_t>(), "set key range BEGIN value")
//            ("to", po::value<uint64_t>(), "set key range END value")
//            ("encrypted", po::value<std::string>(), "encrypted file path")
//            ("decrypt", po::value<std::string>(), "decrypted file path")
//            ;
//
//    po::variables_map vm;
//    po::store(po::parse_command_line(argc, argv, desc), vm);
//    po::notify(vm);
//
//    if (vm.count("help")) {
//        std::cout << desc << std::endl;
//        return 1;
//    }
//
//    if (vm.count("from"))
//        options.absolute_key_from = vm["from"].as<uint64_t>();
//    else {
//        std::cerr << "Set min range!" << std::endl;
//        return 1;
//    }
//
//    if (vm.count("to"))
//        options.absolute_key_to = vm["to"].as<uint64_t>();
//    else {
//        std::cerr << "Set max range!" << std::endl;
//        return 1;
//    }
//
//    if (vm.count("encrypted")) {
//        ::encrypted_file = vm["encrypted"].as<std::string>();
//    } else {
//        std::cerr << "Encrypted file path missing!" << std::endl;
//        return 1;
//    }
//
//    if (vm.count("decrypt")) {
//        ::decrypt_file = vm["decrypt"].as<std::string>();
//    } else {
//        std::cout << "Decrypt file path not set... Using default: " << ::decrypt_file << std::endl;
//    }
//
//    assert(options.absolute_key_from >= 0 && options.absolute_key_to > 0);
//    assert(options.absolute_key_from < options.absolute_key_to);
//
//    mpi::environment env(mpi::threading::multiple, true);
//    mpi::communicator world;
//
//    Master master;
//    master.init();
//
//    Slave slave;
//    slave.init();
//
//    if (world.rank() == 0) {
//
//        //MPI messages queues
//        boost::lockfree::spsc_queue<MpiMessage> receive_queue(128);
//        boost::lockfree::spsc_queue<MpiMessage> send_queue(128);
//
//        auto thread_pool = boost::thread::hardware_concurrency();
//        auto used_threads = 2; // MPI_thread, Process_thread
//
//        //MPI message id - autoincrement
//        boost::atomic_uint32_t mpi_message_id{0};
//
//        //up and running (working) slaves
//        boost::container::vector<int> working_node;
//
//        //work map: slave -> {thread_range...}
//        boost::container::map<rank_type, boost::container::vector<std::pair<uint64_t, uint64_t>>> workmap;
//
//        //watchdog:
//        boost::shared_mutex watchdog_mutex;
//        //  <message_id, <node, kick_number>>
//        boost::container::map<uint32_t, std::pair<rank_type, int>> watchdog_need_callback{};
//
//        //found key
//        uint64_t match_found = 0;
//
//        auto process_thread_implementation = [&]{
//
//            uint64_t received = 0l;
//            while (isAlive){
//
//                auto message_processing = [&] {
//                    MpiMessage msg;
//                    while (receive_queue.pop(msg)) {
//
//                        switch (msg.event){
//                            case MpiMessage::Event::CALLBACK:{
//
//                                MpiMessage::Callback callback_msg_id;
//                                if (msg.respond_to)
//                                    callback_msg_id = *(msg.respond_to);
//                                else
//                                    throw std::runtime_error{"Cannot get message_to optional!"};
//
//                                switch (callback_msg_id.event){
//                                    case MpiMessage::Event::INIT:{
//
//                                    }break;
//
//                                    case MpiMessage::Event::PING:{
//
//                                    }break;
//                                    default:{
//                                    }break;
//                                }
//
//                            }break;
//
//                            case MpiMessage::Event::FOUND:{
//
//
//                            }break;
//
//                            case MpiMessage::Event::SLAVE_DONE:{
//
//                            }break;
//
//                            case MpiMessage::Event::SLAVE_WORKER_DONE:{
//
//                            }break;
//
//                            default:{
//                            }break;
//                        }
//
//                    }
//                    boost::this_thread::sleep_for(boost::chrono::nanoseconds(process_message_loop_delay));
//                };
//
//                message_processing();
//
//            }
//        };
//
//        boost::thread process_thread(process_thread_implementation);
//
//        for (int i = 1; i < world.size(); i++) {
//
//
//        }
//
//        while (isAlive) {
////            boost::optional<mpi::status> stat = world.iprobe(mpi::any_source, mpi::any_tag);
////            if (stat){
////                MpiMessage received_message;
////                world.recv((*stat).source(), (*stat).tag(), received_message);
////                receive_queue.push(boost::move(received_message));
////            }
////
////            if (!send_queue.empty()){
////                MpiMessage msg;
////                while (send_queue.pop(msg)){
////                    world.send(msg.receiver, 0, msg);
////                }
////            }
//            boost::this_thread::sleep_for(boost::chrono::nanoseconds(receive_thread_loop_delay));
//        }
//
//        process_thread.join();
//
//        exit(EXIT_SUCCESS);
//
//    } else {
//
//        boost::lockfree::spsc_queue<MpiMessage> receive_queue(128);
//        boost::lockfree::spsc_queue<MpiMessage> send_queue(128);
//
//        auto thread_pool = boost::thread::hardware_concurrency();
//        auto used_threads = 2; // MPI_thread, Process_thread
//
//        uint32_t mpi_message_id = 0;
//        boost::shared_ptr<syscom_message_counter> syscom_message_counter_ptr = boost::make_shared<syscom_message_counter>();
//
//        auto process_thread_implementation = [&]{
//
//            auto avaiable_threads = thread_pool - used_threads;
//
//            boost::container::vector<boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>>> syscom_thread_tx{};
//            boost::container::vector<boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>>> syscom_thread_rx{};
//
//            boost::container::vector<boost::shared_ptr<boost::thread>> thread_array{};
//
//            boost::container::vector<boost::shared_ptr<Worker>> worker_pointers{};
//
//            boost::container::vector<std::pair<int, bool>> worker_running{};
//
//            boost::container::vector<std::pair<uint64_t, uint64_t>> worker_ranges{};
//
//            bool isInit = false;
//
//            MpiMessage ping_msg;
//            boost::container::vector<uint64_t> ping_info(avaiable_threads); // <worker, key_value>
//            int collected = 0;
//
//            auto init_worker_threads = [&]{
//                for (int i = 0; i < avaiable_threads; i++){
//
//                    boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>> sc_tx = boost::make_shared<boost::lockfree::spsc_queue<SysComMessage>>(128);
//                    syscom_thread_tx.push_back(sc_tx);
//
//                    boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>> sc_rx = boost::make_shared<boost::lockfree::spsc_queue<SysComMessage>>(128);
//                    syscom_thread_rx.push_back(sc_rx);
//
//                    auto thread_task = [i, sc_rx, sc_tx, syscom_message_counter_ptr, &worker_pointers, &worker_ranges]{
//                        boost::shared_ptr<Worker> worker_ptr = boost::make_shared<Worker>(i, Worker::KeyRange{worker_ranges[i].first, worker_ranges[i].second}, ::encrypted_file, ::decrypt_file, sc_rx, sc_tx, syscom_message_counter_ptr);
//                        worker_pointers.push_back(worker_ptr);
//                        worker_ptr->start();
//                    };
//                    boost::shared_ptr<boost::thread> worker_thread_ptr = boost::make_shared<boost::thread>(thread_task);
//                    thread_array.push_back(worker_thread_ptr);
//
//                    worker_running.emplace_back(i, true);
//                }
//            };
//
//            auto process_syscom_message = [&](SysComMessage& sys_msg) {
//                switch (sys_msg.event) {
//                    case SysComMessage::Event::PING: {
//
//                    }break;
//
//                    case SysComMessage::Event::CALLBACK: {
//                        SysComMessage::Callback cb = *(sys_msg.respond_to);
//
//                        switch (cb.event) {
//                            case SysComMessage::Event::PING: {
//
//                            }break;
//
//                            case SysComMessage::Event::INTERRUPT: {
//
//                            }break;
//
//                            default: {
//                            }break;
//                        }
//
//                    }break;
//
//                    case SysComMessage::Event::WORKER_DONE: {
//
//                    }break;
//
//                    case SysComMessage::Event::KEY_FOUND: {
//
//                    }break;
//
//                    default: {
//                    }break;
//                }
//            };
//
//            auto message_processing = [&] {
//                uint64_t received = 0;
//
//                while (isAlive) {
//
//                    MpiMessage msg;
//
//                    while (receive_queue.pop(msg)) {
//                        switch (msg.event) {
//                            case MpiMessage::Event::PING: {
//
//                            }break;
//
//                            case MpiMessage::Event::INIT: {
//
//                            }break;
//
//                            case MpiMessage::Event::KILL: {
//
//                            }break;
//
//                            case MpiMessage::Event::SLAVE_REARRANGE:{
//                            }break;
//
//                            default: {
//                            }break;
//                        }
//                    }
//
//                    for (auto& comm : syscom_thread_rx) {
//                        SysComMessage sys_msg_;
//
//                        if (comm->pop(sys_msg_))
//                            process_syscom_message(sys_msg_);
//                    }
//
//                    boost::this_thread::sleep_for(boost::chrono::nanoseconds(process_message_loop_delay));
//                }
//
//            };
//
//            message_processing();
//        };
//
//        boost::thread process_thread(process_thread_implementation);
//        process_thread.detach();
//
//        while (isAlive) {
////            boost::optional<mpi::status> stat = world.iprobe(mpi::any_source, mpi::any_tag);
////            if (stat){
////                MpiMessage received_message;
////                world.recv((*stat).source(), (*stat).tag(), received_message);
////                receive_queue.push(boost::move(received_message));
////            }
////
////            if (!send_queue.empty()){
////                MpiMessage msg;
////                while (send_queue.pop(msg)){
////                    world.send(msg.receiver, 0, msg);
////                }
////            }
//            boost::this_thread::sleep_for(boost::chrono::nanoseconds(receive_thread_loop_delay));
//        }
//
//    }
//
//    return EXIT_SUCCESS;
//}
