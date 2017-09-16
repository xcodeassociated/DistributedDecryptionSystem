//
// Created by Janusz Majchrzak on 14/09/2017.
//

#include <fstream>
#include "FileExceptions.hpp"
#include <RunOptions.hpp>

namespace File {

    bool file_empty(const std::string& file_name) {
        std::ifstream file(file_name, std::ios::binary);
        if (!file)
            throw FileNotAccessibleException{"Cannot check_open: " + file_name};

        return file.peek() == std::ifstream::traits_type::eof();
    }

    void check_open(const std::string &file_name) {
        std::ofstream fs(file_name, std::ios::binary);
        if (!fs.is_open())
            throw FileNotAccessibleException{"File: " + file_name + " cannot be open/created"};
    }

    void check_from_run_parameters(const RunParameters &parameters) {
        if (file_empty(parameters.encrypted_file))
            throw FileEmptyException{"File: " + parameters.encrypted_file + " empty"};

        check_open(parameters.decrypted_file);
        check_open(parameters.progress_dump_file);

        if (!parameters.progress_file.empty())
            if (file_empty(parameters.encrypted_file))
                throw FileEmptyException{"File: " + parameters.encrypted_file + " empty"};
    }

}