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
    std::string progress_dump_file = "DDS_Progress.txt";
    int message_polling_timeout = 10000000;         // 10s -- default
    int slave_polling_rate = 3000000;    // 3s -- default
};

namespace RunOptions {

    RunParameters get(int argc, const char* argv[]);

}

#endif //DDS_RUNOPTIONS_HPP
