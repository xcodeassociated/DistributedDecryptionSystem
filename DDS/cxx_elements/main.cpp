#include <iostream>
//#include <experimental/optional>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "utilities/utils.hpp"

#include <boost/mpi.hpp>
#include <boost/serialization/string.hpp>
#include <boost/program_options.hpp>

#include "MasterMainClass.hpp"
#include "SlaveMainClass.hpp"

namespace mpi = boost::mpi;
namespace po = boost::program_options;

//#define CRYPTO
//#define DEBUG

#ifdef DEBUG
    #define RANK 0
#endif

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

namespace uj {
    using thread = unsigned;
    using node_id = unsigned int;
    using time_stamp = std::size_t; //TODO: Change to some boost time type
    using nodes = std::vector<node_id>;
    using threads = std::vector<thread>;
    
    template<typename T>
    struct message {
        struct message_body {
            T _data;
            node_id receiver;
            node_id sender;
            time_stamp send_time;
        };
        message_body body;
        
        message() = default;
        
        message(message_body && mb) : body{mb} {};
        
        operator T &() { return body._data; };
        
        friend class boost::serialization::access;
        
        template<typename Ar>
        void serialize(Ar &ar, const unsigned int version) { //TODO: add more data for serialization - data form message_body
            ar & body._data;
        }
        
    };
    
}
TASK_LIMIT_TYPE NUMBER_OF_JOBS;

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
    
    core::MasterMainClass   master;
    core::SlaveMainClass    slave;
    
    (void)master.init();
    (void)slave.init();
    
#ifdef DEBUG
    std::cout << "-- DEBUG, RANK: " << RANK << std::endl;
#endif
    
#ifndef DEBUG
    
    mpi::environment env(mpi::threading::multiple, true);
    (void)env;
    mpi::communicator world;
    
#endif
    
#ifndef DEBUG
    if (world.rank() == 0) {
#else
    if (RANK == 0){
#endif
        
#ifndef DEBUG
        /* ====================== Master MPI ====================== */
       
        /* ====================== ~(Master MPI) ====================== */
#endif
        
    } else {
#ifndef DEBUG
        /* ====================== Slave MPI ====================== */
      
        
        /* ====================== ~(Slave MPI) ====================== */

#endif
    }
    
    return EXIT_SUCCESS;
}
