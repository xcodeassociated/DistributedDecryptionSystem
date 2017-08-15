#include <iostream>
#include <string>
#include <exception>
#include <stdexcept>

#include <boost/shared_ptr.hpp>
#include <boost/program_options.hpp>
#include <boost/mpi.hpp>

#include "ping.cpp"
#include <Master.hpp>
#include <Slave.hpp>

namespace mpi = boost::mpi;
namespace po = boost::program_options;

constexpr auto master_delay = 10u;

int main(int argc, const char* argv[]) {
    std::cout << std::boolalpha;
    std::cout.sync_with_stdio(true);

    boost::shared_ptr<mpi::environment> env = boost::make_shared<mpi::environment>(mpi::threading::single, true);
    boost::shared_ptr<mpi::communicator> world = boost::make_shared<mpi::communicator>();

    if (world->rank() == 0) {

        uint64_t range_begine = 0;
        uint64_t range_end = 0;
        std::string encrypted_file = "";
        std::string decrypted_file = "";

        po::options_description desc("DDS Options");
        desc.add_options()
                ("help", "produce help MpiMessage")
                ("from", po::value<uint64_t>(), "set key range BEGIN value")
                ("to", po::value<uint64_t>(), "set key range END value")
                ("encrypted", po::value<std::string>(), "encrypted file path")
                ("decrypt", po::value<std::string>(), "decrypted file path");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return EXIT_FAILURE;
        }

        if (vm.count("from"))
            range_begine = vm["from"].as<uint64_t>();
        else {
            std::cerr << "Set min range!" << std::endl;
            return EXIT_FAILURE;
        }

        if (vm.count("to"))
            range_end = vm["to"].as<uint64_t>();
        else {
            std::cerr << "Set max range!" << std::endl;
            return EXIT_FAILURE;
        }

        if (vm.count("encrypted")) {
            encrypted_file = vm["encrypted"].as<std::string>();
        } else {
            std::cerr << "Encrypted file path missing!" << std::endl;
            return EXIT_FAILURE;
        }

        if (vm.count("decrypt")) {
            decrypted_file = vm["decrypt"].as<std::string>();
        } else {
            std::cout << "Decrypt file path not set... Using default: " << decrypted_file << std::endl;
        }


        Master master(world);
        master.init(range_begine, range_end);

    } else {

        Slave slave(world);
        slave.init();
    }

    return EXIT_SUCCESS;
}