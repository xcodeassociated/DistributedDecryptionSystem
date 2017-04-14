#include <iostream>
#include <string>
#include <sstream>

#include <boost/shared_ptr.hpp>
#include <boost/container/vector.hpp>
#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/chrono.hpp>
#include <boost/mpi.hpp>
#include <boost/serialization/string.hpp>
#include <boost/lockfree/spsc_queue.hpp>

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


//TODO: ?? Polimprphism for messages ??

struct MpiMessage{
    enum class Event{
        INIT,
        PING,
        FOUND,
        KILL
    };
    rank_type receiver;
    rank_type sender;
    data_type data;
    Event event;
    
    MpiMessage() = default;
    MpiMessage(rank_type _receiver, rank_type _sender, Event _event, data_type data)
            : receiver{_receiver}, sender{_sender}, event{_event}, data{data} {}
    MpiMessage(const MpiMessage&) = default;

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        ar & receiver;
        ar & sender;
        ar & data;
        ar & event;
    }
};

struct SysComMessage{
    using DataType = std::string;
    enum class Event{
        KEY_FOUND,
        INTERRUPT,
        PING
    };
    Event event;
    rank_type id;
    DataType data;
    
    SysComMessage() = default;
    SysComMessage(int _id, Event _evt, DataType _data) : id{_id}, event{_evt}, data{_data} {}
    SysComMessage(const SysComMessage&) = default;
};

auto constexpr receive_thread_loop_delay    = 100000u;         /* ns -> 0.1ms */
auto constexpr process_message_loop_delay   = 10000000u;       /* ns  -> 10ms */

struct Options{
    static int foo;
};
int Options::foo = 0;

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
    uint64_t current_key = 0;
    uint32_t modulo_key = 0;
    boost::lockfree::spsc_queue<SysComMessage>& syscom;
    
public:
    explicit Worker(int _id, boost::lockfree::spsc_queue<SysComMessage>& syscom_ref)
            : id{_id}, syscom{syscom_ref} {}
    
    explicit Worker(int _id, KeyRange _range, uint32_t _modulo, boost::lockfree::spsc_queue<SysComMessage>& syscom_ref)
            : id{_id}, range(_range), modulo_key{_modulo}, syscom{syscom_ref} {}
    
    void setRange(KeyRange&& _range){
        this->range = _range;
    }
    
    void setRange(uint64_t _begin, uint64_t _end){
        this->setRange({_begin, _end});
    }
    
    auto getRange(void) -> KeyRange {
        return this->range;
    }
    
    void setModuloKey(uint32_t _modulo){
        this->modulo_key = _modulo;
    }
    
    auto getModuloKey(void) -> uint32_t {
        return this->modulo_key;
    }
    
    //TODO: CryptoCPP
    void start(){
        assert(this->modulo_key > 0);
        
        if (!this->work)
            this->work = true;
        
        while (this->work){
            this->current_key++;
            
            if ((this->current_key % this->modulo_key) == 0) {
                SysComMessage syscom_msg{this->id, SysComMessage::Event::PING, std::to_string(this->current_key)};
                this->syscom.push(syscom_msg);
            }
            boost::this_thread::sleep_for(boost::chrono::nanoseconds(5));
        }
    }
    
    void stop(){
        this->work = true;
    }

};

int main(int argc, const char* argv[]) {
    std::cout << std::boolalpha;
    std::cout.sync_with_stdio(false);
    
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
        auto used_threads = 2u; // MPI TH, Process TH
        
        std::cout << "[debug: " << world.rank() << "] Thread pool: " << thread_pool << std::endl;
        std::cout << "[debug: " << world.rank() << "] Master (MPI) main thread id: " << boost::this_thread::get_id() << std::endl;
        
        auto process_thread_implementation = [&]{
            std::cout << "[debug: " << world.rank() << "] Process thread id: " <<  boost::this_thread::get_id() <<  " has been triggered!" << std::endl;
            
            std::size_t received = 0l;
            while (true){
                
                auto init = [&] {
                    // TODO: total key range calc
                    // TODO: prepare key ranges to send as well as modulo for workers
                    // TODO: watchdog service enable (checking if slave is up & getting current results)
                };
                
                auto message_processing = [&] {
                    MpiMessage msg;
                    while (receive_queue.pop(msg)) {
                        std::cout << "[debug: " << world.rank() << "]: Processing... (received " << ++received
                                  << ") MpiMessage: \n";
                        std::cout << "      {" << "sender: " << msg.receiver << ", data: " << msg.data << "}"
                                  << std::endl;
                        {
                            std::stringstream ss;
                            ss << "SENDING BACK FROM: " << world.rank() << std::endl;
                            data_type data{ss.str()};
            
                            send_queue.push({msg.receiver, world.rank(), MpiMessage::Event::PING, data});
                        }
                    }
                    boost::this_thread::sleep_for(boost::chrono::nanoseconds(process_message_loop_delay));
                };
                
                init();
                //...
                message_processing();
                
            }
        };
        
        boost::thread process_thread(process_thread_implementation);
        process_thread.detach();
        
        boost::this_thread::sleep_for(boost::chrono::seconds(2));
        std::cout << "[debug: " << world.rank() << "] " << "----- Init ping pong squence: STARTS -----" << std::endl;
        for (int i = 1; i < world.size(); i++) {
            std::cout << "[debug: " << world.rank() << "] " << "Sending init data to: " << i << std::endl;
            send_queue.push({i, world.rank(), MpiMessage::Event::INIT, "INIT FROM MASTER"});
        }
        std::cout << "[debug: " << world.rank() << "] " << "----- Init ping pong squence: ENDS -----" << std::endl;
        
        while (true) {
            boost::optional<mpi::status> stat = world.iprobe(mpi::any_source, mpi::any_tag);
            if (stat){
                std::cout << "[debug: " << world.rank() << "] Receive thread has probed a MpiMessage..."<< std::endl;
                
                //data_type data{""};
                MpiMessage received_message;
                world.recv((*stat).source(), (*stat).tag(), received_message);
                receive_queue.push({(*stat).source(), world.rank(), MpiMessage::Event::PING, received_message.data});
                
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
        auto used_threads = 2u; // MPI TH, Process TH
        
        std::cout << "[debug: " << world.rank() << "] Thread pool: " << thread_pool << std::endl;
        std::cout << "[debug: " << world.rank() << "] Master (MPI) main thread id: " << boost::this_thread::get_id() << std::endl;
        
        auto process_thread_implementation = [&]{
            std::cout << "[debug: " << world.rank() << "] Process thread id: " <<  boost::this_thread::get_id() <<  " has been triggered!" << std::endl;
            
            boost::lockfree::spsc_queue<SysComMessage> syscom(128);
            
            auto avaiable_threads = thread_pool - used_threads;
            boost::container::vector<boost::shared_ptr<Worker>> worker_array(avaiable_threads);
            boost::container::vector<boost::thread> thread_array{};
            boost::condition_variable barrier;
            boost::mutex barrier_mutex;
            bool barrier_flag = false;
            
            uint8_t worker_id = 0u;
            
            auto init_worker_threads = [&]{
                // TODO: devide key pool for threads...
                
                for (auto& worker : worker_array){
                    std::cout << "[debug: " << world.rank() << "] Preparing Worker for thread: " << worker_id << std::endl;
                    worker = boost::make_shared<Worker>(worker_id++, Worker::KeyRange{0, 1000}, 5000, syscom);
                    auto thread_task = [&]{
                        boost::unique_lock<boost::mutex> lk(barrier_mutex);
                        barrier.wait(lk, [&barrier_flag]{ return barrier_flag; });
                        
                        worker->start();
                    };
                    thread_array.emplace_back(thread_task);
                }
                
                barrier_flag = true;
                barrier.notify_all();
            };
            
            auto message_processing = [&] {
                std::size_t received = 0l;
                while (true) {
                    MpiMessage msg;
                    while (receive_queue.pop(msg)) {
                        std::cout << "[debug: " << world.rank() << "]: Processing... (received " << ++received
                                  << ") MpiMessage: \n";
                        std::cout << "      {" << "sender: " << msg.receiver << ", data: " << msg.data << "}"
                                  << std::endl;
                        {
                            std::stringstream ss;
                            ss << "SENDING BACK FROM: " << world.rank() << std::endl;
                            data_type data{ss.str()};
                
                            //...
                            
                            send_queue.push({msg.receiver, world.rank(), MpiMessage::Event::PING, data});
                        }
                    }
                    
                    SysComMessage sys_msg;
                    while (syscom.pop(sys_msg)){
                        switch (sys_msg.event){
                            case SysComMessage::Event::PING:
                                std::cout << "[debug: " << world.rank() << "] SysCom: PING EVENT: "
                                          << " {id:" << sys_msg.id << ", data: " << sys_msg.data  << "}"<< std::endl;
                                break;
                        }
                    }
                    
                    boost::this_thread::sleep_for(boost::chrono::nanoseconds(process_message_loop_delay));
                }
            };
            
            init_worker_threads();
            //...
            message_processing();
        };
        
        boost::thread process_thread(process_thread_implementation);
        process_thread.detach();
        
        while (true) {
            boost::optional<mpi::status> stat = world.iprobe(mpi::any_source, mpi::any_tag);
            if (stat){
                std::cout << "[debug: " << world.rank() << "] Receive thread has probed a MpiMessage..."<< std::endl;
                
                //data_type data{""};
                MpiMessage received_message;
                world.recv((*stat).source(), (*stat).tag(), received_message);
                receive_queue.push({(*stat).source(), world.rank(), MpiMessage::Event::PING, received_message.data});
                
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
