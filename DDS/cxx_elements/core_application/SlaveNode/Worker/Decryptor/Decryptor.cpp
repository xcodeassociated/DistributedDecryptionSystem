//
// Created by Janusz Majchrzak on 30/05/17.
//

#include <sstream>
#include <utility>
#include <fstream>
#include <exception>
#include <boost/container/vector.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>

#include <cryptopp/sha.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>

#include "Decryptor.hpp"

#include <boost/chrono.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/thread.hpp>

Decryptor::Decryptor(boost::shared_ptr<boost::lockfree::spsc_queue<SysComSTR>> _rx,
                     boost::shared_ptr<boost::lockfree::spsc_queue<SysComSTR>> _tx) : WorkerBase(_rx, _tx) {
    ;
}

void Decryptor::load_data_buffer() {
    std::ifstream file_stream(this->file_path, std::ios::binary);
    std::stringstream read_stream;
    read_stream << file_stream.rdbuf();
    std::string read_stream_str = read_stream.str();

    boost::container::vector<std::string> file_lines;
    boost::split(file_lines, read_stream_str, boost::is_any_of("\n"));

    std::string sha1 = file_lines[0];
    std::string ciphertext = "";
    for (std::size_t i = 1; i < file_lines.size(); i++) {
        ciphertext += file_lines[i];
        if (i < file_lines.size() - 2)
            ciphertext += '\n';
    }

    this->encryptd_data = ciphertext;
    this->encrypted_sha = sha1;
}

void Decryptor::init_decryptor(int _id, KeyRange _range, std::string _file_path, std::string _decrypted_path){
    this->id = _id;
    this->range = _range;
    this->file_path = _file_path;
    this->decrypted_file_path = _decrypted_path;
    this->current_key = this->range.begin;

    this->load_data_buffer();
}

boost::container::vector<unsigned char> Decryptor::uint64ToBytes(uint64_t value) noexcept {
    boost::container::vector<unsigned char> result(8, 0x00);
    result.push_back((value >> 56) & 0xFF);
    result.push_back((value >> 48) & 0xFF);
    result.push_back((value >> 40) & 0xFF);
    result.push_back((value >> 32) & 0xFF);
    result.push_back((value >> 24) & 0xFF);
    result.push_back((value >> 16) & 0xFF);
    result.push_back((value >>  8) & 0xFF);
    result.push_back((value) & 0xFF);
    return result;
}

std::string Decryptor::hashString(const std::string& str) noexcept {
    std::string result;
    CryptoPP::SHA1 sha1;
    CryptoPP::StringSource(str, true,
                           new CryptoPP::HashFilter(sha1, new CryptoPP::HexEncoder(
                                   new CryptoPP::StringSink(result), true)));
    return result;
}

void Decryptor::process_syscom() {
    SysComSTR sysmsg;
    while (this->rx->pop(sysmsg)){
        switch (sysmsg.type){
            case SysComSTR::Type::PING: {
                std::string str_key = boost::lexical_cast<std::string>(this->current_key);
                this->tx->push({str_key, SysComSTR::Type::CALLBACK});
            }break;

            case SysComSTR::Type::KILL: {
                this->stop();
            }break;

            default: { ; } break;
        }
    }
}

void Decryptor::notify_key_found(uint64_t key){
    std::string str_key = boost::lexical_cast<std::string>(key);
    this->tx->push({str_key, SysComSTR::Type::FOUND});
}

bool Decryptor::decrypt() {
    auto key_bytes = this->uint64ToBytes(this->current_key);

    assert(key_bytes.size() == CryptoPP::AES::DEFAULT_KEYLENGTH);
    byte* key = key_bytes.data();

    byte keyArray[CryptoPP::AES::BLOCKSIZE];
    memset(keyArray, 0x00, CryptoPP::AES::BLOCKSIZE);

    CryptoPP::AES::Decryption decryption_engine(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(decryption_engine, keyArray);

    try {
        std::string decrypted_text = "";
        CryptoPP::StreamTransformationFilter st_filter(cbcDecryption, new CryptoPP::StringSink(decrypted_text));
        st_filter.Put(reinterpret_cast<const unsigned char *>(this->encryptd_data.c_str()), this->encryptd_data.size());
        st_filter.MessageEnd();

        if (decrypted_text.back() == '\0')
            decrypted_text.pop_back();

        std::string decrypted_sha = this->hashString(decrypted_text);

        if (decrypted_sha == this->encrypted_sha) {

            if (!this->decrypted_file_path.empty()){
                std::ofstream os(this->decrypted_file_path, std::ios::binary);
                os << decrypted_text;
                os.flush();
                os.close();
            }

            return true;
        }


    } catch (const CryptoPP::InvalidCiphertext &e) {
        ;
    } catch (...) {
        ;
    }

    return false;
}

void Decryptor::worker_process() {
    this->work = true;

    while (this->work){
        this->process_syscom();

        if (this->done || this->found) {
            boost::this_thread::sleep(boost::posix_time::microseconds(100));
            continue;
        } else {
            if (!this->decrypt()) {
                if (this->current_key < this->range.end)
                    this->current_key += 1;
                else
                    this->done = true;
            } else {
                this->found = true;
                this->notify_key_found(this->current_key);
            }
        }
    }
}

