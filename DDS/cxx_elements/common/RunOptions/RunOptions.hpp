//
// Created by Janusz Majchrzak on 16/08/2017.
//

#ifndef DDS_RUNOPTIONS_HPP
#define DDS_RUNOPTIONS_HPP

#include <string>
#include <cstdint>

struct RunParameters {
    uint64_t range_begine = 0;
    uint64_t range_end = 0;
    std::string encrypted_file = "";
    std::string decrypted_file = "";
    std::string progress_file = "";
    std::string hosts_file = "";
};

namespace RunOptions {

    RunParameters get(int argc, const char* argv[]);

};


#endif //DDS_RUNOPTIONS_HPP
