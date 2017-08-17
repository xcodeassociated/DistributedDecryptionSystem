//
// Created by Janusz Majchrzak on 30/05/17.
//

#ifndef DDS_WORKERIMPLEMENTATION_HPP
#define DDS_WORKERIMPLEMENTATION_HPP

#include <string>
#include <utility>

#include <WorkerBase.hpp>

using byte = unsigned char;

class Decryptor : public WorkerBase {

    std::string file_path = "";
    std::string decrypted_file_path = "";
    std::string encryptd_data = "";

    bool init_decryptor(int, KeyRange, std::string, std::string);

public:

    template <class ... T>
    bool init(T&... args) {
        return this->init_decryptor(std::forward<T>(args)...);
    }

    void worker_process() override;

    boost::container::vector<unsigned char> uint64ToBytes(uint64_t value) noexcept;
    std::string hashString(const std::string& str) noexcept;

    Decryptor();
};


#endif //DDS_WORKERIMPLEMENTATION_HPP
