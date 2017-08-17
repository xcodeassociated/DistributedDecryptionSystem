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

        po::options_description desc("DDS RunOptions");
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
            std::stringstream ss;
            ss << desc;
            throw IncorrectParameterException{ss.str()};
        }

        if (vm.count("from"))
            runParameters.range_begine = vm["from"].as<uint64_t>();
        else {
            throw MissingParameterException{"Set min range!"};
        }

        if (vm.count("to"))
            runParameters.range_end = vm["to"].as<uint64_t>();
        else {
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

        //
    }

}