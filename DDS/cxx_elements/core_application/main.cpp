#include <iostream>
#include <string>
#include <sstream>
#include <utility>
#include <fstream>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <fstream>

#include <boost/container/vector.hpp>
#include <boost/container/set.hpp>
#include <boost/container/map.hpp>

#include <boost/filesystem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/atomic.hpp>
#include <boost/mpi.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/optional.hpp>
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

#include "ping.cpp"
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
    boost::shared_ptr<syscom_message_counter> syscom_message_counter_ptr = nullptr;

    std::string file_path = "";
    std::string decrypted_file_path = "";
    static boost::mutex mu;

public:

    explicit Worker(int _id, KeyRange _range, std::string _file_path, std::string _decrypted_path)
            : id{_id},
              range(_range),
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

            if (this->work) {
                if (this->current_key == this->range.end + 1) {
                    this->stop();

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

std::string encrypted_file = "";
std::string decrypt_file = "decrypted.txt";

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Gateway {

    static void _send(boost::shared_ptr<mpi::communicator> world, const int rank, const int tag, const std::string &msg){
        boost::posix_time::ptime begin = boost::posix_time::microsec_clock::local_time();
        mpi::request send_request = world->isend(rank, tag, msg);
        while (true){
            if (send_request.test())
                break;

            boost::posix_time::ptime end = boost::posix_time::microsec_clock::local_time();
            boost::posix_time::time_duration duration = end - begin;
            if (duration.total_microseconds() >= 100000)  // wait 1.5s to send
                throw std::runtime_error{"Send timeout"};
        }
    }

    static boost::optional<std::string> _receive(boost::shared_ptr<mpi::communicator> world, const int rank, const int tag){
        std::string data = "";
        boost::posix_time::ptime begin_probe = boost::posix_time::microsec_clock::local_time();
        while (true) {
                boost::optional<mpi::status> stat = world->iprobe(rank, tag);
                if (stat) {
                    boost::posix_time::ptime begin_receive = boost::posix_time::microsec_clock::local_time();
                    mpi::request recv_request = world->irecv(stat->source(), stat->tag(), data);
                    while (true) {
                        if (recv_request.test())
                            return boost::optional<std::string>{data};

                        boost::posix_time::ptime end_receive = boost::posix_time::microsec_clock::local_time();
                        boost::posix_time::time_duration duration_receive = end_receive - begin_receive;
                        if (duration_receive.total_microseconds() >= 100000)  // wait 1.5s to receive message
                            throw std::runtime_error{"Receive timeout"};

                    }
                }
            boost::posix_time::ptime end_probe = boost::posix_time::microsec_clock::local_time();
            boost::posix_time::time_duration duration_probe = end_probe - begin_probe;

            if (duration_probe.total_microseconds() >= 1000000)  // wait 1s to probe message
                break;
        }
        return {};
    }

public:

    static void ping(const int rank){
        //get rank ip address
        const std::string hosts_file_name = "hosts";

        if (!boost::filesystem::exists(hosts_file_name))
            throw std::runtime_error{"Hosts file not found"};

        std::ifstream hosts_file(hosts_file_name, std::ios::binary);
        std::vector<std::string> hosts_ip;
        std::copy(std::istream_iterator<std::string>(hosts_file),
                  std::istream_iterator<std::string>(),
                  std::back_inserter(hosts_ip));

        std::string ip;
        try {
            ip = hosts_ip[rank];
        } catch (const std::out_of_range &) { // change std::out_of_range exception into my own exception class
            throw std::runtime_error("Rank not present in hosts file");
        } catch (...) {
            throw std::runtime_error("Unknow exception");
        }

        boost::asio::io_service io_service;
        pinger p(io_service, ip.c_str());
        io_service.run_one(); // <--- blocking operation - will throw if 3s timeout reached
    }

    static void send(boost::shared_ptr<mpi::communicator> world, const int rank, const int tag, const std::string &msg){
        ping(rank);
        _send(world, rank, tag, msg);
    }

    static boost::optional<std::string> receive(boost::shared_ptr<mpi::communicator> world, const int rank, const int tag) {
        ping(rank);
        return _receive(world, rank, tag);
    }

    static boost::optional<std::string> send_and_receive(boost::shared_ptr<mpi::communicator> world, const int rank, const int tag, const std::string &msg) {
        ping(rank);
        _send(world, rank, tag, msg);
        return _receive(world, rank, tag);
    }

    static void unsafe_send(boost::shared_ptr<mpi::communicator> world, const int rank, const int tag, const std::string &msg){
        _send(world, rank, tag, msg);
    }

    static boost::optional<std::string> unsafe_receive(boost::shared_ptr<mpi::communicator> world, const int rank, const int tag) {
        return _receive(world, rank, tag);
    }

};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr auto master_delay = 10u;

int main(int argc, const char* argv[]) {
    std::cout << std::boolalpha;
    std::cout.sync_with_stdio(true);

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
        return EXIT_FAILURE;
    }

    if (vm.count("from"))
        (void)vm["from"].as<uint64_t>();
    else {
        std::cerr << "Set min range!" << std::endl;
        return EXIT_FAILURE;
    }

    if (vm.count("to"))
        (void)vm["to"].as<uint64_t>();
    else {
        std::cerr << "Set max range!" << std::endl;
        return EXIT_FAILURE;
    }

    if (vm.count("encrypted")) {
        ::encrypted_file = vm["encrypted"].as<std::string>();
    } else {
        std::cerr << "Encrypted file path missing!" << std::endl;
        return EXIT_FAILURE;
    }

    if (vm.count("decrypt")) {
        ::decrypt_file = vm["decrypt"].as<std::string>();
    } else {
        std::cout << "Decrypt file path not set... Using default: " << ::decrypt_file << std::endl;
    }

    boost::shared_ptr<mpi::environment> env = boost::make_shared<mpi::environment>(mpi::threading::single, true);
    boost::shared_ptr<mpi::communicator> world = boost::make_shared<mpi::communicator>();
    boost::container::vector<int> excluded{};

    if (world->rank() == 0) {
        while (true) {
            if (!world)
                break;

            for (int i = 1; i < world->size(); i++) {
                if (std::find(excluded.begin(), excluded.end(), i) != excluded.end())
                    continue;

                boost::posix_time::ptime begin = boost::posix_time::microsec_clock::local_time();
                try {

                    boost::optional<std::string> response = Gateway::send_and_receive(world, i, 0, "hello");

                    if (response) {
                        std::cout << "[" << world->rank() << "]: Master received: " << *response; //<< std::endl;
                    } else {
                        std::cerr << "[" << world->rank()
                                  << "]: Master did NOT received exception, but option is empty!"; // << std::endl;
                    }
                } catch (const std::runtime_error &e) {
                    std::cerr << "[" << world->rank() << "]: Master exception: " << e.what() << " - " << i << std::endl;
                    excluded.push_back(i);
                }
                boost::posix_time::ptime end = boost::posix_time::microsec_clock::local_time();
                boost::posix_time::time_duration duration = end - begin;

                std::cout << " @time: " << duration.total_milliseconds() << "ms" << std::endl;

                boost::this_thread::sleep(boost::posix_time::milliseconds(master_delay));
            }
        }
    } else {
        while (true) {
            if (!world)
                break;

            try {
                boost::optional<std::string> message = Gateway::unsafe_receive(world, 0, 0);
                if (message) {
                    Gateway::unsafe_send(world, 0, 0, std::to_string(world->rank()));
                } else {
                    std::cerr << "[" << world->rank() << "]: " << " Slave received EMPTY message!" << std::endl;
                }
            } catch (const std::runtime_error& e) {
                std::cerr << "[" << world->rank() << "]: " << " Slave received exception: " << e.what() << std::endl;
            }
        }
    }

    return EXIT_SUCCESS;
}