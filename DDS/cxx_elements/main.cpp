#include <iostream>
#include <string>
#include <sstream>
#include <utility>

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
#include <cryptopp/modes.h>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#endif
    
namespace mpi = boost::mpi;
namespace po = boost::program_options;

using data_type = std::string;
using rank_type = int;
using message_id_type = uint32_t;

struct PingWorkerReport{

};

struct MpiMessage{
    enum class Event : int {
        CALLBACK = 0,
        INIT = 1,
        PING = 2,
        FOUND = 3,
        KILL = 4,
        SLAVE_DONE = 5,
        SLAVE_REARRANGE = 6
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
    
public:
    explicit Worker(int _id,
                    boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>> syscom_tx_ref,
                    boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>> syscom_rx_ref,
                    boost::shared_ptr<syscom_message_counter> counter_ptr)
            : id{_id},
              syscom_tx_ptr{syscom_tx_ref},
              syscom_rx_ptr{syscom_rx_ref},
              syscom_message_counter_ptr{counter_ptr}
    {}
    
    explicit Worker(int _id, KeyRange _range,
                    boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>> syscom_tx_ref,
                    boost::shared_ptr<boost::lockfree::spsc_queue<SysComMessage>> syscom_rx_ref,
                    boost::shared_ptr<syscom_message_counter> counter_ptr)
            : id{_id},
              range(_range),
              syscom_tx_ptr{syscom_tx_ref},
              syscom_rx_ptr{syscom_rx_ref},
              syscom_message_counter_ptr{counter_ptr}
    {}
    
    void setRange(KeyRange&& _range){
        this->range = _range;
    }
    
    void setRange(uint64_t _begin, uint64_t _end){
        this->setRange({_begin, _end});
    }
    
    auto getRange(void) -> KeyRange {
        return this->range;
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
                        this->work = false;
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
                if (this->current_key == this->range.end) {
                    this->work = false;
                    SysComMessage syscom_msg{(*this->syscom_message_counter_ptr)++, this->id, SysComMessage::Event::WORKER_DONE, false, "done"};
                    this->syscom_tx_ptr->push(syscom_msg);
                }else {
                    // ...
                    this->current_key++;
                }
            }

            boost::this_thread::sleep_for(boost::chrono::nanoseconds(5));
        }
    }

    // TODO: CryptoCPP
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
        this->work = true;
    }

    void finish(){
        this->process = false;
    }

};

auto constexpr receive_thread_loop_delay    = 100000u;         /* ns -> 0.1ms */
auto constexpr process_message_loop_delay   = 10000000u;       /* ns  -> 10ms */

struct Options{
    static int foo;
};
int Options::foo = 0;

int main(int argc, const char* argv[]) {
    std::cout << std::boolalpha;
    std::cout.sync_with_stdio(true);
    
    po::options_description desc("DDS Options");
    desc.add_options()
            ("help", "produce help MpiMessage")
            ("foo", po::value<int>(), "set `foo` level");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }
    if (vm.count("foo")) {
        Options::foo = vm["foo"].as<int>();
        std::cout << "[debug] PreMPI: `foo` level was set to: " << Options::foo << std::endl;
    } else {
        std::cout << "[debug] PreMPI: `foo` level was not set! " << std::endl;
        return 1;
    }
    
    mpi::environment env(mpi::threading::multiple, true);
    mpi::communicator world;

    if (world.rank() == 0) {

        boost::lockfree::spsc_queue<MpiMessage> receive_queue(128);
        boost::lockfree::spsc_queue<MpiMessage> send_queue(128);
        
        auto thread_pool = boost::thread::hardware_concurrency();
        auto used_threads = 2; // MPI_thread, Process_thread

        boost::atomic_uint32_t mpi_message_id{0};

        boost::container::map<rank_type, boost::container::map<int, uint64_t>> nodemap; // node - workers (progress)
        boost::container::map<uint32_t, std::pair<rank_type, int>> watchdog_need_callback{}; // <message_id, <node, kick_number>>
        boost::container::vector<int> ready_node;

        std::cout << "[debug: " << world.rank() << "] Thread pool: " << thread_pool << std::endl;
        std::cout << "[debug: " << world.rank() << "] Master (MPI) main thread rank: " << boost::this_thread::get_id() << std::endl;
        
        auto process_thread_implementation = [&]{
            std::cout << "[debug: " << world.rank() << "] Process thread rank: " <<  boost::this_thread::get_id() <<  " has been triggered!" << std::endl;
            
            uint64_t received = 0l;
            while (true){

                // TODO: total key range calc
                // TODO: prepare key ranges to send as well as modulo for workers
                // TODO: watchdog service enable (checking if slave is up & getting current results)

                // TODO: define number of checks for a message after which watchdog will disable sending messages to node.
                // TODO: verify progress of another pings for rank... if there will be no progress between given number of pings also assume that something went wrong and disable node
                // TODO: evry disabling of node has to trigger logic to move that filed node tasks to other nodes
                boost::function<void(void)> watchdog_implementation{[&](void) -> void {
//                    std::cout << "[debug: " << world.rank() << "] Watchod triggered!" << std::endl;
//                    std::cout << "[debug: " << world.rank() << "] Watchod goes a sleep..." << std::endl;
                }};

                CallBackTimer watchdog;
                watchdog.start(boost::chrono::milliseconds(5000), watchdog_implementation);

                boost::function<void(void)> pinger_implementation{[&](void) -> void {
                    // TODO: similar code used in init... don't copy code!!!
                    for (int i = 1; i < world.size(); i++) {
                        if (std::find(ready_node.begin(), ready_node.end(), i) != ready_node.end()) {
                            auto it = std::find_if(watchdog_need_callback.begin(), watchdog_need_callback.end(),
                                                   [&](const auto &e) {
                                                       return (e.second).first == i;
                                                   });
                            if (it == watchdog_need_callback.end()) {
                                auto msg_id = mpi_message_id++;
                                send_queue.push({msg_id, i, world.rank(), MpiMessage::Event::PING, true, "?"});

                                // TODO: watchdog register mechanizm
                                if (watchdog_need_callback.size() > 0) {
                                    if (watchdog_need_callback.find(msg_id) == watchdog_need_callback.end())
                                        watchdog_need_callback[msg_id] = std::make_pair(i, 0);
                                    else
                                        throw std::runtime_error{"Message already registered in Watchdog!"};
                                } else
                                    watchdog_need_callback[msg_id] = std::make_pair(i, 0);
                            } else {
                                std::cout << "[debug: " << world.rank() << "] kick!!" << std::endl;
                                for (auto t : watchdog_need_callback) {
                                    std::cout << "{" << t.first << ", " << t.second.first << ", " << t.second.second << "}\n";
                                }
                            }
                        }
                    }
                }};
                CallBackTimer pinger;
                pinger.start(boost::chrono::milliseconds(2000), pinger_implementation);


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

                                MpiMessage::Callback callback_msg_id;
                                if (msg.respond_to)
                                    callback_msg_id = *(msg.respond_to);
                                else
                                    throw std::runtime_error{"Cannot get message_to optional!"};

                                std::cout << "[debug: " << world.rank() << "]: Received CALLBACK message with id: " << msg.id << " for {msg_id: " << callback_msg_id.message_id << ", event: " << std::to_string(
                                        static_cast<int>(callback_msg_id.event)) << "} form rank: " << msg.sender << std::endl;

                                // TODO: unregister message form watchdog
                                auto watchdog_element_iterator = watchdog_need_callback.find(callback_msg_id.message_id);
                                if (watchdog_element_iterator != watchdog_need_callback.end()) {
                                    watchdog_need_callback.erase(watchdog_element_iterator);
                                    std::cout << "[debug: " << world.rank() << "]: Watchdog unregistered message: {" << callback_msg_id.message_id << ", event: " << std::to_string(
                                            static_cast<int>(callback_msg_id.event)) << "}" << std::endl;
                                }else{
                                    std::cout << "[error: " << world.rank() << "]: Watchdog could not find message: " << callback_msg_id.message_id << std::endl;
                                    // TODO: what next?
                                }

                                //TODO: Process callback...
                                switch (callback_msg_id.event){
                                    case MpiMessage::Event::INIT:{
                                        std::cout << "[info: " << world.rank() << "]: - - - - FINISHED INIT FOR node: " << msg.sender << std::endl;

                                        if (nodemap.find(msg.sender) != nodemap.end())
                                            throw std::runtime_error{"Error mapping node with workers..."};

                                        int workers = std::atoi(msg.data.c_str());

                                        std::cout << "[debug: " << world.rank() << "]: Setting number of workers for node: " << msg.sender << " on: " << std::to_string(workers) << std::endl;

                                        boost::container::map<int, uint64_t> temp{};
                                        for (int i = 0; i < workers; i++){
                                            temp[i] = 0;
                                        }

                                        assert(static_cast<int>(temp.size()) == workers);

                                        nodemap[msg.sender] = temp;
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

                            }break;
                            case MpiMessage::Event::SLAVE_DONE:{
                                std::cout << "[info: " << world.rank() << "]: Master received SLAVE_DONE from: " << msg.sender << std::endl;
                                // TODO: mark slave node as offline - by intention. (not because of slave is down exidently)
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
        process_thread.detach();
        
        boost::this_thread::sleep_for(boost::chrono::seconds(2));
        std::cout << "[debug: " << world.rank() << "] " << "----- Init ping pong squence: STARTS -----" << std::endl;
        for (int i = 1; i < world.size(); i++) {
            std::cout << "[debug: " << world.rank() << "] " << "Sending init data to: " << i << std::endl;
            auto msg_id = mpi_message_id++;
            send_queue.push({msg_id, i, world.rank(), MpiMessage::Event::INIT, true, "INIT FROM MASTER"});

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
        
        while (true) {
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

            boost::container::vector<std::pair<int, bool>> worker_up{};

            boost::container::vector<Worker::KeyRange> new_key_ranges{};

            bool isInit = false;
            bool isKilled = false;

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

                    auto thread_task = [i, sc_rx, sc_tx, syscom_message_counter_ptr, &worker_pointers]{
                        boost::shared_ptr<Worker> worker_ptr = boost::make_shared<Worker>(i, Worker::KeyRange{0, 2000000}, sc_rx, sc_tx, syscom_message_counter_ptr);
                        worker_pointers.push_back(worker_ptr);
                        worker_ptr->start();
                    };
                    boost::shared_ptr<boost::thread> worker_thread_ptr = boost::make_shared<boost::thread>(thread_task);
                    thread_array.push_back(worker_thread_ptr);
                    worker_up.emplace_back(i, true);
                }
            };

            auto process_syscom_message = [&](SysComMessage sys_msg) {
                switch (sys_msg.event) {
                    // TODO: When ping is trigger by modulo - send message to master node or store last key for faster access? Something to concider...
                    case SysComMessage::Event::PING: {
                        std::cout << "[debug: " << world.rank() << "] SysCom: PING EVENT: "
                                  << "{rank:" << sys_msg.rank << ", data: " << sys_msg.data << "}"
                                  << std::endl;

                    }break;

                        //When there's a callback from worker e.g. ping callback
                    case SysComMessage::Event::CALLBACK: {
                        SysComMessage::Callback cb = *(sys_msg.respond_to);

                        std::cout << "[debug: " << world.rank() << "] SysCom: CALLBACK EVENT: "
                                  << "{rank:" << sys_msg.rank << " respons to: {message_id: "
                                  << cb.message_id << ", type: " << std::to_string(
                                static_cast<int>(cb.event)) << "}" << ", data: " << sys_msg.data << "}"
                                  << std::endl;

                        switch (cb.event) {
                            case SysComMessage::Event::PING: {
                                std::cout << "[debug: " << world.rank()
                                          << "] SysCom: Processing CALLBACK event for PING: "
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
                                              << "] SysCom: Processing CALLBACK event for PING: "
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
                                std::cout << "[debug: " << world.rank() << "] SysCom: Processing CALLBACK event for INTERRUPT form Worker: " << sys_msg.rank << std::endl;
                                std::cout << "[debug: " << world.rank() << "] SysCom: Worker: " << sys_msg.rank << " is turned off!" << std::endl;

                                auto it = std::find(worker_up.begin(), worker_up.end(), std::pair<int, bool>(sys_msg.rank, true));
                                if (it == worker_up.end())
                                    throw std::runtime_error{"0x003"};
                                *it = std::make_pair(sys_msg.rank, false);

                                int still_up = 0;
                                for (const auto& element : worker_up){
                                    if (element.second)
                                        still_up++;
                                }

                                if (still_up > 0)
                                    std::cout << "[debug: " << world.rank() << "] SysCom: There are(is) still: " << still_up << " worker(s) up." << std::endl;
                                else {
                                    std::cout << "[debug: " << world.rank() << "] SysCom: There are NO workers up! Slave node: " << world.rank()  << " is about to close - Sending message to Master" << std::endl;

                                    send_queue.push({mpi_message_id++, 0, world.rank(), MpiMessage::Event::SLAVE_DONE, false, "slave_done"});
                                    // TODO: if there are no workers up - slave goes down
                                }
                            }break;

                            default: {
                                std::cout << "[error: " << world.rank() << "] Unknow SYSCOM CALLBACK message!" << std::endl;
                            }break;
                        }

                    }break;

                    case SysComMessage::Event::WORKER_DONE: {
                        std::cout << "[debug: " << world.rank() << "] SysCom: Processing *** WORKER DONE for Worker thread: " << sys_msg.rank << " ***"<< std::endl;

                        if (new_key_ranges.size() > 0){
                            std::cout << "[debug: " << world.rank() << "] SysCom: Assigning a new key range to worker..." << std::endl;
                            // TODO: if the worker has finished it's own job, assigne a new key range to process...
                        }else {
                            std::cout << "[debug: " << world.rank() << "] SysCom: There is no need for handle new key range. Sending INTERRUPT to worker: "
                                      << sys_msg.rank << " because the thread has finished." << std::endl;

                            SysComMessage syscom_msg{(*syscom_message_counter_ptr)++, sys_msg.rank, SysComMessage::Event::INTERRUPT, true, "interrupt"};
                            syscom_thread_tx[sys_msg.rank]->push(syscom_msg);
                        }
                    }break;

                    default: {
                        std::cout << "[error: " << world.rank() << "] Unknow SYSCOM message!" << std::endl;
                    }break;
                }
            };

            auto message_processing = [&] {
                uint64_t received = 0;

                while (true) {

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

                            }
                                break;
                            case MpiMessage::Event::INIT: {
                                if (!isInit) {
                                    std::cout << "[debug: " << world.rank() << "]: Init message processing..."
                                              << std::endl;

                                    // TODO: devide key pool for threads...
                                    // ...

                                    init_worker_threads();

                                    std::stringstream ss;
                                    ss << avaiable_threads << std::endl;
                                    data_type data{ss.str()};
                                    send_queue.push({mpi_message_id++, msg.sender, world.rank(), MpiMessage::Event::CALLBACK,
                                             false, data, MpiMessage::Callback{msg.id, msg.event}});
                                } else {
                                    // TODO: send invalide operation...
                                }
                            }
                                break;
                            case MpiMessage::Event::KILL: {
                                if (!isKilled) {
                                    std::stringstream ss;
                                    ss << "KILL << SENDING BACK FROM: " << world.rank() << " >>" << std::endl;
                                    data_type data{ss.str()};
                                    send_queue.push(
                                            {mpi_message_id++, msg.sender, world.rank(), MpiMessage::Event::CALLBACK,
                                             false, data, MpiMessage::Callback{msg.id, msg.event}});
                                } else {
                                    // TODO: send invalide operation...
                                }
                            }
                                break;
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
        
        while (true) {
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
    }
    
    return EXIT_SUCCESS;
}
