//
// Created by Janusz Majchrzak on 16/08/2017.
//

#include <iostream>
#include <sstream>
#include <boost/program_options.hpp>

#include "RunOptions.hpp"
#include "RunOptionsExceptions.hpp"

namespace po = boost::program_options;

namespace RunOptions {

    RunParameters get(int argc, const char* argv[]) {
        RunParameters runParameters;

        try {
            po::options_description desc("DDS RunOptions");
            desc.add_options()
                    ("help", "produce help MpiMessage")
                    ("from", po::value<uint64_t>(), "set key range BEGIN value")
                    ("to", po::value<uint64_t>(), "set key range END value")
                    ("encrypted", po::value<std::string>(), "encrypted file path")
                    ("decrypt", po::value<std::string>(), "decrypted file path")
                    ("progress_dump", po::value<std::string>(), "file that holds progress dump")
                    ("resume", po::value<std::string>(), "resume work from progress file")
                    ("message_timeout", po::value<int>(), "set max time (in microseconds) when Master waits for response from "
                            "Slave after sending request -- 10s is default value")
                    ("slave_polling_rate", po::value<int>(), "set Slave progress and events polling (in microseconds) -- 3s is default value");

            po::variables_map vm;
            po::store(po::parse_command_line(argc, argv, desc), vm);
            po::notify(vm);

            bool resume = false;

            if (vm.count("help")) {
                std::stringstream ss;
                ss << desc;
                throw IncorrectParameterException{ss.str()};
            }

            if (vm.count("resume")) {
                runParameters.progress_file = vm["resume"].as<std::string>();
                resume = true;
            }

            if (vm.count("message_timeout")) {
                int timeout = vm["message_timeout"].as<int>();
                if (timeout > 0)
                    runParameters.message_polling_timeout = timeout;
                else
                    throw IncorrectParameterException{"message_timeout parameter must be grater than 0, passed: " + std::to_string(timeout)};
            }

            if (vm.count("slave_polling_rate")) {
                int slave_polling = vm["slave_polling_rate"].as<int>();
                if (slave_polling > 0)
                    runParameters.slave_polling_rate = slave_polling;
                else
                    throw IncorrectParameterException{"slave_polling_rate parameter must be grater than 0, passed: " + std::to_string(slave_polling)};
            }

            if (vm.count("from") || resume) {
                if (!resume)
                    runParameters.range_begine = vm["from"].as<uint64_t>();
            } else {
                throw MissingParameterException{"Set min range!"};
            }

            if (vm.count("to") || resume) {
                if (!resume)
                    runParameters.range_end = vm["to"].as<uint64_t>();
            } else {
                throw MissingParameterException{"Set max range!"};
            }

            if (vm.count("encrypted")) {
                runParameters.encrypted_file = vm["encrypted"].as<std::string>();
            } else {
                throw MissingParameterException{"Encrypted file path missing!"};
            }

            if (vm.count("decrypt")) {
                runParameters.decrypted_file = vm["decrypt"].as<std::string>();
            }

            if (vm.count("progress_dump")) {
                runParameters.progress_dump_file = vm["progress_dump"].as<std::string>();
            }
        } catch (const po::error& e) {
            throw RunOptionException{e.what()};
        }

        return runParameters;
    }

}