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
    std::string encrypted_sha = "";

    bool init_decryptor(int, KeyRange, std::string, std::string);
    void load_data_buffer();
    virtual void process_syscom() override;
    void notify_key_found(uint64_t);

public:
    using WorkerBase::WorkerBase;

    template <class ... T>
    bool init(T&... args) {
        return this->init_decryptor(std::forward<T>(args)...);
    }

    void worker_process() override;

    boost::container::vector<unsigned char> uint64ToBytes(uint64_t value) noexcept;
    std::string hashString(const std::string& str) noexcept;

    Decryptor(boost::shared_ptr<boost::lockfree::spsc_queue<SysComSTR>>,
              boost::shared_ptr<boost::lockfree::spsc_queue<SysComSTR>>);

    virtual ~Decryptor() = default;
};


#endif //DDS_WORKERIMPLEMENTATION_HPP
