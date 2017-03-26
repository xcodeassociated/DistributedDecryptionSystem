#include <iostream>
#include <sstream>
#include <thread>
#include <atomic>
#include <mutex>
// ?? #include <shared_mutex>
#include <condition_variable>

#include "utilities/utils.hpp"

#include <boost/mpi.hpp>
#include <boost/serialization/string.hpp>
#include <boost/program_options.hpp>
#include <boost/optional.hpp>

#include "MasterMainClass.hpp"
#include "SlaveMainClass.hpp"

namespace mpi = boost::mpi;
namespace po = boost::program_options;
using namespace std::chrono_literals;

//#define CRYPTO

#ifdef CRYPTO
#include "modes.h"
#include "aes.h"
#include "filters.h"
#endif

void crypto() {
#ifdef CRYPTO
    
    std::cout << "------------------------------------------------------------------" << endl;
    std::cout << "Crypto++: " << endl;
    //Key and IV setup
    //AES encryption uses a secret key of a variable length (128-bit, 196-bit or 256-
    //bit). This key is secretly exchanged between two parties before communication
    //begins. DEFAULT_KEYLENGTH= 16 bytes
    byte key[ CryptoPP::AES::DEFAULT_KEYLENGTH ], iv[ CryptoPP::AES::BLOCKSIZE ];
    memset( key, 0x00, CryptoPP::AES::DEFAULT_KEYLENGTH );
    memset( iv, 0x00, CryptoPP::AES::BLOCKSIZE );
    
    //
    // String and Sink setup
    //
    std::string plaintext = "Now is the time for all good men to come to the aide...";
    std::string ciphertext;
    std::string decryptedtext;
    
    //
    // Dump Plain Text
    //
    std::cout << "Plain Text (" << plaintext.size() << " bytes)" << endl;
    std::cout << plaintext;
    std::cout << endl << endl;
    
    //
    // Create Cipher Text
    //
    CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption( aesEncryption, iv );
    
    CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink( ciphertext ) );
    stfEncryptor.Put( reinterpret_cast<const unsigned char*>( plaintext.c_str() ), plaintext.length() + 1 );
    stfEncryptor.MessageEnd();
    
    //
    // Dump Cipher Text
    //
    std::cout << "Cipher Text (" << ciphertext.size() << " bytes)" << endl;
    
    for( int i = 0; i < ciphertext.size(); i++ ) {
        
        std::cout << "0x" << std::hex << (0xFF & static_cast<byte>(ciphertext[i])) << " ";
    }
    
    std::cout << endl << endl;
    
    //
    // Decrypt
    //
    CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption( aesDecryption, iv );
    
    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink( decryptedtext ) );
    stfDecryptor.Put( reinterpret_cast<const unsigned char*>( ciphertext.c_str() ), ciphertext.size() );
    stfDecryptor.MessageEnd();
    
    //
    // Dump Decrypted Text
    //
    std::cout << "Decrypted Text: " << endl;
    std::cout << decryptedtext;
    std::cout << endl << endl;
    
    std::cout.flush();
    std::cout << "------------------------------------------------------------------" << endl;
    
#endif
}

using TASK_LIMIT_TYPE = uint64_t;

TASK_LIMIT_TYPE NUMBER_OF_JOBS;

using data_type = std::string;



int main(int argc, const char* argv[]) {
    std::cout << std::boolalpha;
    std::cout.sync_with_stdio(false);
    
    po::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("example_param", po::value<TASK_LIMIT_TYPE>(), "set compression level")
            ;
    
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
    
    if (vm.count("help")) {
        std::cout << desc << endl;
        return 1;
    }
    if (vm.count("example_param")) {
        NUMBER_OF_JOBS = vm["example_param"].as<TASK_LIMIT_TYPE>();
    
        std::cout << "example_param level was set to: " << NUMBER_OF_JOBS << endl;
    } else {
        std::cout << "example_param level was not set! " << endl;
        return 1;
    }
    
  
    mpi::environment env(mpi::threading::multiple, true);
    (void)env;
    mpi::communicator world;
    
    if (world.rank() == 0) {
        
        core::MasterMainClass master;
        (void)master.init();
    
        auto main_thread_id = std::this_thread::get_id();
        std::cout << "[debug] Master main thread id: " << main_thread_id << std::endl;
        
        std::mutex m;
        std::condition_variable cv;
        bool ready = false;
      
        auto receive_thread_implementation = [&world, &master, &ready, &m, &cv]{
            {
                std::lock_guard<std::mutex> lg(m);
                auto thread_id = std::this_thread::get_id();
                std::cout << "[debug] Receive thread id: " << thread_id << std::endl;
                std::cout << "[debug] Receive thread waits for trigger..." << std::endl;
            }
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, [&ready]{return ready;});
            
            std::cout << "[debug] Receive thread waits for triggered!" << std::endl;
            
            boost::optional<mpi::status> stat;
            data_type data{""};
            while (true) {
                stat = world.iprobe(mpi::any_source, mpi::any_tag);
                if (stat){
                    mpi::request req = world.irecv((*stat).source(), (*stat).tag(), data);

                    if (req.test()){
                        std::cout << "[debug] Receive thread received message!!!\n    data:" << data << std::endl;

                    }
                    
                    stat.reset();
                    data = {};
                }
                std::this_thread::sleep_for(100ms);
            }
            
        };
        
        std::thread receive_thread(receive_thread_implementation);
        
        std::cout << "[debug] Main thread goes to sleep for 2s..." << std::endl;
        std::this_thread::sleep_for(2s);
        std::cout << "[debug] Main thread wakes up!" << std::endl;
        
        {
            std::lock_guard< std::mutex > lk(m);
            ready = true;
        }
        cv.notify_one();
        
        
        //TODO
        
        receive_thread.join();
        
        std::cout << "[debug] Main thread has been joined with receive thread." << std::endl;
        
    } else {
    
        core::SlaveMainClass slave;
        (void)slave.init();
    
        auto main_thread_id = std::this_thread::get_id();
        std::cout << "[debug] Slave [" << world.rank() << "]: main thread id: " << main_thread_id << std::endl;
        
        //std::this_thread::sleep_for(std::chrono::seconds(3 + world.rank()));
        std::cout << "[debug] Slave rank: " << world.rank() << " send message!" << std::endl;
        
        std::stringstream ss;
        ss << "message: Hello form slave: " << world.rank() << std::endl;
        data_type data{ss.str()};
        mpi::request send_rq = world.isend(0, 0, data);
        send_rq.wait();

    }
    
    return EXIT_SUCCESS;
}
