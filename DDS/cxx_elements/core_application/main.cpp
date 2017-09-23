#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdint>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/mpi.hpp>

#include <RunOptions.hpp>
#include <RunOptionsExceptions.hpp>
#include <FileCheck.cpp>
#include <FileExceptions.hpp>
#include <Master.hpp>
#include <MasterExceptions.hpp>
#include <Slave.hpp>
#include <SlaveExceptions.hpp>
#include <Gateway.hpp>

namespace mpi = boost::mpi;

int main(int argc, const char* argv[]) {
    std::cout << std::boolalpha;
    std::cout.sync_with_stdio(true);

    boost::shared_ptr<mpi::environment> env = boost::make_shared<mpi::environment>(mpi::threading::single, true);
    boost::shared_ptr<mpi::communicator> world = boost::make_shared<mpi::communicator>();
    RunParameters runParameters;

    try {
        runParameters = RunOptions::get(argc, argv);
    } catch (const MissingParameterException& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (const IncorrectParameterException& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (const RunOptionException& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    try {
        File::check_from_run_parameters(runParameters);
    } catch (const FileNotAccessibleException& err) {
        std::cerr << err.what() << std::endl;
        return EXIT_FAILURE;
    } catch (const FileEmptyException& err) {
        std::cerr << err.what() << std::endl;
        return EXIT_FAILURE;
    } catch (const FileException& err) {
        std::cerr << err.what() << std::endl;
        return EXIT_FAILURE;
    }

    Gateway::set_message_polling_timeout(runParameters.message_polling_timeout);

    if (world->rank() == 0) {

        try {
            std::cout << "Setting Gateway message polling timeout for message request response: "
                          << runParameters.message_polling_timeout << " microseconds" << std::endl;
            std::cout << "Setting Slave polling rate: " << runParameters.slave_polling_rate << " microseconds" << std::endl;

            Master::set_slave_polling_rate(runParameters.slave_polling_rate);

            std::cout << "Work progress will be stored in file: " << runParameters.progress_dump_file << std::endl;

            Master master(world, runParameters.progress_dump_file);
            if (runParameters.progress_file.empty()) {
                if (master.init(runParameters.range_begine, runParameters.range_end))
                    master.start();
            } else {
                if (master.init(runParameters.progress_file))
                    master.start();
            }
        } catch (const MasterException& e) {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        } catch (const FileException& e) {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }

    } else {

        try {
            Slave slave(world, runParameters.encrypted_file, runParameters.decrypted_file);
            slave.init();
            slave.start();
        } catch (const SlaveException& e) {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }

    }

    return EXIT_SUCCESS;
}