//
// Created by Janusz Majchrzak on 14/09/2017.
//

#include <fstream>
#include "FileExceptions.hpp"
#include <RunOptions.hpp>

namespace Files {

    bool file_empty(const std::string& file_name) {
        std::ifstream file(file_name, std::ios::binary);
        if (!file)
            throw FileNotAccessibleException{"Cannot open: " + file_name};

        return file.peek() == std::ifstream::traits_type::eof();
    }

    void open(const std::string& file_name) {
        std::ofstream fs(file_name, std::ios::binary);
        if (!fs.is_open())
            throw FileNotAccessibleException{"File: " + file_name + " cannot be open/created"};
    }

    void check(const RunParameters& parameters) {
        if (file_empty(parameters.encrypted_file))
            throw FileEmptyException{"File: " + parameters.encrypted_file + " empty"};

        open(parameters.decrypted_file);
        open(parameters.progress_dump_file);

        if (!parameters.progress_file.empty())
            if (file_empty(parameters.encrypted_file))
                throw FileEmptyException{"File: " + parameters.encrypted_file + " empty"};
    }

}