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

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <istream>
#include <iostream>
#include <ostream>

class icmp_header
{
public:
    enum { echo_reply = 0, destination_unreachable = 3, source_quench = 4,
        redirect = 5, echo_request = 8, time_exceeded = 11, parameter_problem = 12,
        timestamp_request = 13, timestamp_reply = 14, info_request = 15,
        info_reply = 16, address_request = 17, address_reply = 18 };

    icmp_header() { std::fill(rep_, rep_ + sizeof(rep_), 0); }

    unsigned char type() const { return rep_[0]; }
    unsigned char code() const { return rep_[1]; }
    unsigned short checksum() const { return decode(2, 3); }
    unsigned short identifier() const { return decode(4, 5); }
    unsigned short sequence_number() const { return decode(6, 7); }

    void type(unsigned char n) { rep_[0] = n; }
    void code(unsigned char n) { rep_[1] = n; }
    void checksum(unsigned short n) { encode(2, 3, n); }
    void identifier(unsigned short n) { encode(4, 5, n); }
    void sequence_number(unsigned short n) { encode(6, 7, n); }

    friend std::istream& operator>>(std::istream& is, icmp_header& header)
    { return is.read(reinterpret_cast<char*>(header.rep_), 8); }

    friend std::ostream& operator<<(std::ostream& os, const icmp_header& header)
    { return os.write(reinterpret_cast<const char*>(header.rep_), 8); }

private:
    unsigned short decode(int a, int b) const
    { return (rep_[a] << 8) + rep_[b]; }

    void encode(int a, int b, unsigned short n)
    {
        rep_[a] = static_cast<unsigned char>(n >> 8);
        rep_[b] = static_cast<unsigned char>(n & 0xFF);
    }

    unsigned char rep_[8];
};

template <typename Iterator>
void compute_checksum(icmp_header& header,
                      Iterator body_begin, Iterator body_end)
{
    unsigned int sum = (header.type() << 8) + header.code()
                       + header.identifier() + header.sequence_number();

    Iterator body_iter = body_begin;
    while (body_iter != body_end)
    {
        sum += (static_cast<unsigned char>(*body_iter++) << 8);
        if (body_iter != body_end)
            sum += static_cast<unsigned char>(*body_iter++);
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    header.checksum(static_cast<unsigned short>(~sum));
}

class ipv4_header
{
public:
    ipv4_header() { std::fill(rep_, rep_ + sizeof(rep_), 0); }

    unsigned char version() const { return (rep_[0] >> 4) & 0xF; }
    unsigned short header_length() const { return (rep_[0] & 0xF) * 4; }
    unsigned char type_of_service() const { return rep_[1]; }
    unsigned short total_length() const { return decode(2, 3); }
    unsigned short identification() const { return decode(4, 5); }
    bool dont_fragment() const { return (rep_[6] & 0x40) != 0; }
    bool more_fragments() const { return (rep_[6] & 0x20) != 0; }
    unsigned short fragment_offset() const { return decode(6, 7) & 0x1FFF; }
    unsigned int time_to_live() const { return rep_[8]; }
    unsigned char protocol() const { return rep_[9]; }
    unsigned short header_checksum() const { return decode(10, 11); }

    boost::asio::ip::address_v4 source_address() const
    {
        boost::asio::ip::address_v4::bytes_type bytes
                = { { rep_[12], rep_[13], rep_[14], rep_[15] } };
        return boost::asio::ip::address_v4(bytes);
    }

    boost::asio::ip::address_v4 destination_address() const
    {
        boost::asio::ip::address_v4::bytes_type bytes
                = { { rep_[16], rep_[17], rep_[18], rep_[19] } };
        return boost::asio::ip::address_v4(bytes);
    }

    friend std::istream& operator>>(std::istream& is, ipv4_header& header)
    {
        is.read(reinterpret_cast<char*>(header.rep_), 20);
        if (header.version() != 4)
            is.setstate(std::ios::failbit);
        std::streamsize options_length = header.header_length() - 20;
        if (options_length < 0 || options_length > 40)
            is.setstate(std::ios::failbit);
        else
            is.read(reinterpret_cast<char*>(header.rep_) + 20, options_length);
        return is;
    }

private:
    unsigned short decode(int a, int b) const
    { return (rep_[a] << 8) + rep_[b]; }

    unsigned char rep_[60];
};

using boost::asio::ip::icmp;
using boost::asio::deadline_timer;
namespace posix_time = boost::posix_time;

class pinger
{
public:
    pinger(boost::asio::io_service& io_service, const char* destination)
            : resolver_(io_service), socket_(io_service, icmp::v4()),
              timer_(io_service), sequence_number_(0), num_replies_(0)
    {
        icmp::resolver::query query(icmp::v4(), destination, "");
        destination_ = *resolver_.resolve(query);

        start_send();
        start_receive();
    }

private:
    void start_send()
    {
        std::string body("\"Hello!\" from Asio ping.");

        // Create an ICMP header for an echo request.
        icmp_header echo_request;
        echo_request.type(icmp_header::echo_request);
        echo_request.code(0);
        echo_request.identifier(get_identifier());
        echo_request.sequence_number(++sequence_number_);
        compute_checksum(echo_request, body.begin(), body.end());

        // Encode the request packet.
        boost::asio::streambuf request_buffer;
        std::ostream os(&request_buffer);
        os << echo_request << body;

        // Send the request.
        time_sent_ = posix_time::microsec_clock::universal_time();
        socket_.send_to(request_buffer.data(), destination_);

        // Wait up to five seconds for a reply.
        num_replies_ = 0;
        timer_.expires_at(time_sent_ + posix_time::seconds(3));
        timer_.async_wait(boost::bind(&pinger::handle_timeout, this));
    }

    void handle_timeout()
    {
        if (num_replies_ == 0)
            throw std::runtime_error("Request timed out");

        // Requests must be sent no less than one second apart.
        timer_.expires_at(time_sent_ + posix_time::seconds(1));
        timer_.async_wait(boost::bind(&pinger::start_send, this));
    }

    void start_receive()
    {
        // Discard any data already in the buffer.
        reply_buffer_.consume(reply_buffer_.size());

        // Wait for a reply. We prepare the buffer to receive up to 64KB.
        socket_.async_receive(reply_buffer_.prepare(65536),
                              boost::bind(&pinger::handle_receive, this, _2));
    }

    void handle_receive(std::size_t length)
    {
        // The actual number of bytes received is committed to the buffer so that we
        // can extract it using a std::istream object.
        reply_buffer_.commit(length);

        // Decode the reply packet.
        std::istream is(&reply_buffer_);
        ipv4_header ipv4_hdr;
        icmp_header icmp_hdr;
        is >> ipv4_hdr >> icmp_hdr;

        // We can receive all ICMP packets received by the host, so we need to
        // filter out only the echo replies that match the our identifier and
        // expected sequence number.
        if (is && icmp_hdr.type() == icmp_header::echo_reply
            && icmp_hdr.identifier() == get_identifier()
            && icmp_hdr.sequence_number() == sequence_number_)
        {
            // If this is the first reply, interrupt the five second timeout.
            if (num_replies_++ == 0)
                timer_.cancel();
        }

        start_receive();
    }

    static unsigned short get_identifier()
    {
#if defined(BOOST_WINDOWS)
        return static_cast<unsigned short>(::GetCurrentProcessId());
#else
        return static_cast<unsigned short>(::getpid());
#endif
    }

    icmp::resolver resolver_;
    icmp::endpoint destination_;
    icmp::socket socket_;
    deadline_timer timer_;
    unsigned short sequence_number_;
    posix_time::ptime time_sent_;
    boost::asio::streambuf reply_buffer_;
    std::size_t num_replies_;
};

#include <thread>
#include <mutex>
#include <condition_variable>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Gateway {

    static std::string _send_and_receive(boost::shared_ptr<mpi::communicator> world, const int rank, const int tag, const std::string &msg){
        std::string data = "";
        mpi::request send_request = world->isend(rank, tag, msg);
        send_request.wait();

        while (true) {
            boost::optional<mpi::status> stat = world->iprobe(rank, tag);
            if (stat) {
                mpi::request recv_request = world->irecv(stat->source(), stat->tag(), data);
                recv_request.wait();
                return data;
            }
        }
    }


public:

    static void ping(const int rank){
        //get rank ip address
        std::ifstream hosts_file("hosts");
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

        //ping ip rank
        boost::asio::io_service io_service;
        pinger p(io_service, ip.c_str());
        io_service.run_one(); // <--- blocking operation - will throw if timeout
    }

    static std::string send_and_receive(boost::shared_ptr<mpi::communicator> world, const int rank, const int tag, const std::string &msg) {
        ping(rank);
        return _send_and_receive(world, rank, tag, msg);
    }

};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

constexpr auto master_delay = 100u;
constexpr auto slave_probe_delay = 10u;

int main(int argc, const char* argv[]) {
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

                boost::chrono::steady_clock::time_point begin = boost::chrono::steady_clock::now();
                try {
                    std::string response = Gateway::send_and_receive(world, i, 0, "hello");

                    if (response != "") {
                        std::cout << "[" << world->rank() << "]: Master received: " << response; //<< std::endl;
                    } else {
                        std::cerr << "[" << world->rank()
                                  << "]: Master did NOT received exception, but option is empty!"; // << std::endl;
                    }
                } catch (const std::runtime_error &e) {
                    std::cerr << "[" << world->rank() << "]: Master exception: " << e.what() << " - " << i << std::endl;
                    excluded.push_back(i);
                }
                boost::chrono::steady_clock::time_point end = boost::chrono::steady_clock::now();

                std::cout << " @time: " << boost::chrono::duration_cast<boost::chrono::milliseconds>(end - begin).count() << "ms" << std::endl;

                boost::this_thread::sleep_for(boost::chrono::milliseconds(master_delay));
            }
        }
    } else {
        std::cout << "Hello form: " << world->rank() << std::endl; int i = 0;
        while (true) {
            if (!world)
                break;

            boost::optional<mpi::status> stat = world->iprobe(0, 0);
            if (stat) {
                std::string msg;
                mpi::request req_rec = world->irecv(stat->source(), stat->tag(), msg);
                req_rec.wait();

                std::cout << "[" << world->rank() << "]: Slave received: " << msg << std::endl;

                std::string response = std::to_string(world->rank());
                mpi::request req_snd = world->isend(0, 0, response);
                req_snd.wait();
            }
            boost::this_thread::sleep_for(boost::chrono::nanoseconds(slave_probe_delay));
        }
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
