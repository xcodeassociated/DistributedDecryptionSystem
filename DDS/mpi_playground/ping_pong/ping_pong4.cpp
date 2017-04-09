#include <iostream>
#include <sstream>

#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <boost/mpi.hpp>
#include <boost/serialization/string.hpp>
#include <boost/lockfree/spsc_queue.hpp>

namespace mpi = boost::mpi;

using data_type = std::string;
using rank_type = int;

template <typename Rank_Type = rank_type, typename Data_Type = data_type>
struct message{
    Rank_Type first;
    Data_Type second;
    virtual ~message() = default;
    message() = default;
    message(Rank_Type rank, Data_Type data) : first{rank}, second{data} {}
    message(const message&) = default;
};

auto constexpr receive_thread_loop_delay = 500u;
auto constexpr process_message_loop_delay = 500u;

int main(int argc, const char* argv[]) {
    std::cout << std::boolalpha;
    std::cout.sync_with_stdio(false);
    
    mpi::environment env;//(mpi::threading::multiple, true);
    mpi::communicator world;
    
    if (world.rank() == 0) {
        
        boost::lockfree::spsc_queue<message<>> receive_queue(128);
        boost::lockfree::spsc_queue<message<>> send_queue(128);
        
        std::cout << "[debug: " << world.rank() << "] Master (MPI) main thread id: " << boost::this_thread::get_id() << std::endl;
        
        auto process_thread_implementation = [&]{
            std::cout << "[debug: " << world.rank() << "] Process thread id: " <<  boost::this_thread::get_id() <<  " has been triggered!" << std::endl;
            
            std::size_t received = 0l;
            while (true){
                message<> msg;
                while (receive_queue.pop(msg)) {
                    std::cout <<"[debug: " << world.rank() << "]: Processing... (received " << ++received << ") message: \n";
                    std::cout << "      {" << "sender: " << msg.first << ", data: " << msg.second << "}" << std::endl;
                    {
                        std::stringstream ss;
                        ss << "SENDING BACK FROM: " << world.rank() << std::endl;
                        data_type data{ss.str()};
                        
                        send_queue.push({msg.first, data});
                    }
                }
                
                if (!send_queue.empty()){
                    message<> msg;
                    while (send_queue.pop(msg)){
                        std::cout << "[debug: " << world.rank() << "] " << "Message {target: " << msg.first << ", data: " << msg.second << "} is about to send" << std::endl;
                        world.send(msg.first, 0, msg.second);
                        std::cout << "[debug: " << world.rank() << "] " << "Message sent." << std::endl;
                    }
                }
                
                boost::this_thread::sleep_for(boost::chrono::milliseconds(process_message_loop_delay));
            }
        };
        
        boost::thread process_thread(process_thread_implementation);
        process_thread.detach();
        
        boost::this_thread::sleep_for(boost::chrono::seconds(2));
        std::cout << "[debug: " << world.rank() << "] " << "----- Init ping pong squence: STARTS -----" << std::endl;
        for (int i = 1; i < world.size(); i++) {
            std::cout << "[debug: " << world.rank() << "] " << "Sending init data to: " << i << std::endl;
            send_queue.push({i, "INIT FROM MASTER"});
        }
        std::cout << "[debug: " << world.rank() << "] " << "----- Init ping pong squence: ENDS -----" << std::endl;
        
        while (true) {
            mpi::status stat = world.probe(mpi::any_source, mpi::any_tag);
            
            std::cout << "[debug: " << world.rank() << "] Receive thread has probed a message..."<< std::endl;
            
            data_type data{""};
            world.recv((stat).source(), (stat).tag(), data);
            receive_queue.push({(stat).source(), data});
            
            std::cout << "[debug: " << world.rank() << "] Receive thread has received message\n    data:" << data << std::endl;
        }
        
    } else {
        
        boost::lockfree::spsc_queue<message<>> receive_queue(128);
        boost::lockfree::spsc_queue<message<>> send_queue(128);
        
        std::cout << "[debug: " << world.rank() << "] Master (MPI) main thread id: " << boost::this_thread::get_id() << std::endl;
        
        auto process_thread_implementation = [&]{
            std::cout << "[debug: " << world.rank() << "] Process thread id: " <<  boost::this_thread::get_id() <<  " has been triggered!" << std::endl;
            
            std::size_t received = 0l;
            while (true){
                message<> msg;
                while (receive_queue.pop(msg)) {
                    std::cout <<"[debug: " << world.rank() << "]: Processing... (received " << ++received << ") message: \n";
                    std::cout << "      {" << "sender: " << msg.first << ", data: " << msg.second << "}" << std::endl;
                    {
                        std::stringstream ss;
                        ss << "SENDING BACK FROM: " << world.rank() << std::endl;
                        data_type data{ss.str()};
                        
                        send_queue.push({msg.first, data});
                    }
                }
                
                if (!send_queue.empty()){
                    message<> msg;
                    while (send_queue.pop(msg)){
                        std::cout << "[debug: " << world.rank() << "] " << "Message {target: " << msg.first << ", data: " << msg.second << "} is about to send" << std::endl;
                        world.send(msg.first, 0, msg.second);
                        std::cout << "[debug: " << world.rank() << "] " << "Message sent." << std::endl;
                    }
                }
                
                boost::this_thread::sleep_for(boost::chrono::milliseconds(process_message_loop_delay));
            }
        };
        
        boost::thread process_thread(process_thread_implementation);
        process_thread.detach();
        
        while (true) {
            mpi::status stat = world.probe(mpi::any_source, mpi::any_tag);
            
            std::cout << "[debug: " << world.rank() << "] Receive thread has probed a message..."<< std::endl;
            
            data_type data{""};
            world.recv((stat).source(), (stat).tag(), data);
            receive_queue.push({(stat).source(), data});
            
            std::cout << "[debug: " << world.rank() << "] Receive thread has received message\n    data:" << data << std::endl;
        }
    }
    
    return EXIT_SUCCESS;
}