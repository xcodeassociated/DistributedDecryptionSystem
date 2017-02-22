#include <iostream>
#include <string>
#include <sstream>
#include "utils.hpp"
#include "Controller.hpp"

#include <boost/mpi.hpp>
#include <boost/serialization/string.hpp>
#include <boost/program_options.hpp>

#include "modes.h"
#include "aes.h"
#include "filters.h"

namespace mpi = boost::mpi;
namespace po = boost::program_options;

#define CRYPTO
//#define DEBUG

#ifdef DEBUG
    #define RANK 0
#endif

using TASK_LIMIT_TYPE = uint64_t;

int main(int argc, const char* argv[]) {
    
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
        std::cout << "example_param level was set to "
             << vm["example_param"].as<TASK_LIMIT_TYPE>() << ".\n";
    } else {
        std::cout << "example_param level was not set.\n";
        return -1;
    }
    
#ifdef DEBUG
    std::cout << "-- DEBUG, RANK: " << RANK << std::endl;
#endif
    
    std::cout << std::boolalpha;
    std::cout.sync_with_stdio(false);
    
    core::Controller c;
    /*std::cout <<*/ c.foo(); /*<< endl;*/
    /*std::cout <<*/ c.inline_foo(); /*<< endl;*/
    
    TASK_LIMIT_TYPE NUMBER_OF_JOBS = vm["example_param"].as<TASK_LIMIT_TYPE>();

#ifndef DEBUG
    mpi::environment env;
    mpi::communicator world;
    
    // Test world size
    if (world.size() > NUMBER_OF_JOBS  + 1) {
        if (world.rank() == 0) {
            std::cerr << "Too many processes (" << world.size()
                      << ") for the number of jobs!\n";
            std::cerr << "Use " << NUMBER_OF_JOBS + 1 << " ranks or less\n";
            return 0;  // Return 0 to avoid openMPI error messages
        } else {
            return 0;  // Return 0 to avoid openMPI error messages
        }
    }
    
    std::cout << "--- [MPI PROCESS]: " << world.rank() << " ---" << endl;

#endif
    
#ifndef DEBUG
    if (world.rank() == 0) {
#else
    if (RANK == 0){
#endif
        
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
        
        std::string msg, out_msg = "Hello";

#ifndef DEBUG
        // Initialize requests
        unsigned int job_id = 0;
        std::vector<mpi::request> reqs(world.size());
    
        // Send initial jobs
        for (unsigned int dst_rank = 1; dst_rank < world.size(); ++dst_rank) {
            std::cout << "[MASTER] Sending job " << job_id
                      << " to SLAVE " <<  dst_rank << "\n";
            // Send job to dst_rank [nonblocking]
            world.isend(dst_rank, 0, job_id);
            // Post receive request for new jobs requests by slave [nonblocking]
            reqs[dst_rank] = world.irecv(dst_rank, 0);
            ++job_id;
        }
    
        // Send jobs as long as there is job left
        while(job_id < NUMBER_OF_JOBS) {
            bool stop;
            for (unsigned int dst_rank = 1; dst_rank < world.size(); ++dst_rank) {
                // Check if dst_rank is done
                if (reqs[dst_rank].test()) {
                    std::cout << "[MASTER] Rank " << dst_rank << " is done.\n";
                    // Check if there is remaining jobs
                    if (job_id  < NUMBER_OF_JOBS) {
                        // Tell the slave that a new job is coming.
                        stop = false;
                        world.isend(dst_rank, 0, stop);
                        // Send the new job.
                        std::cout << "[MASTER] Sending new job (" << job_id
                                  << ") to SLAVE " << dst_rank << ".\n";
                        world.isend(dst_rank, 0, job_id);
                        reqs[dst_rank] = world.irecv(dst_rank, 0);
                        ++job_id;
                    }
                    else {
                        // Send stop message to slave.
                        stop = true;
                        world.isend(dst_rank, 0, stop);
                    }
                }
            }
            usleep(1000);
        }
        std::cout << "[MASTER] Sent all jobs.\n";
    
        // Listen for the remaining jobs, and send stop messages on completion.
        bool all_done = false;
        while (!all_done) {
            all_done = true;
            for (unsigned int dst_rank = 1; dst_rank < world.size(); ++dst_rank) {
                if (reqs[dst_rank].test()) {
                    // Tell the slave that it can exit.
                    bool stop = true;
                    world.isend(dst_rank, 0, stop);
                }
                else {
                    all_done = false;
                }
            }
            usleep(1000);
        }
        std::cout << "[MASTER] Handled all jobs, killed every process.\n";
#endif
        
    } else {
    
        std::string msg, out_msg = "world";

#ifndef DEBUG
        bool stop = false;
        while (!stop) {
            // Wait for new job
            unsigned int job_id = 0;
            world.recv(0, 0, job_id);
            std::cout << "[SLAVE: " << world.rank()
                      << "] Received job " << job_id << " from MASTER.\n";
            // Perform "job"
            int sleep_time = std::rand() / 100000;
            std::cout << "[SLAVE: " << world.rank()
                      << "] Sleeping for " << sleep_time
                      << " microseconds (job " << job_id << ").\n";
            usleep(sleep_time);
            // Notify master that the job is done
            std::cout << "[SLAVE: " << world.rank()
                      << "] Done with job " << job_id << ". Notifying MASTER.\n";
            world.send(0, 0);
            // Check if a new job is coming
            world.recv(0, 0, stop);
        }
#endif
    
    }
    
    return EXIT_SUCCESS;
}