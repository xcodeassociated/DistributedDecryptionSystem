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

#include <cryptopp/sha.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>

#include "Decryptor.hpp"

Decryptor::Decryptor() {
    ;
}

bool Decryptor::init_decryptor(int _id, KeyRange _range, std::string _file_path, std::string _decrypted_path){
    this->id = _id;
    this->range = _range;
    this->file_path = _file_path;
    this->decrypted_file_path = _decrypted_path;

    this->current_key = this->range.begin;
    //TODO: load encrypted data here!
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

void Decryptor::worker_process() {
    while (this->work){
        if (this->current_key == this->range.end + 1)
            this->stop();

            auto key_bytes = this->uint64ToBytes(this->current_key);

            assert(key_bytes.size() == CryptoPP::AES::DEFAULT_KEYLENGTH);
            byte* key = key_bytes.data();

            byte iv[CryptoPP::AES::BLOCKSIZE];
            memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE);

// TODO: make file load only once
std::ifstream file_stream(this->file_path, std::ios::binary);
std::stringstream read_stream;
read_stream << file_stream.rdbuf();
std::string read_stream_str = read_stream.str();

            boost::container::vector<std::string> file_lines;
            boost::split(file_lines, read_stream_str, boost::is_any_of("\n"));

            std::string sha1 = file_lines[0];
            std::string ciphertext = "";
            for (int i = 1; i < file_lines.size(); i++) {
                ciphertext += file_lines[i];
                if (i < file_lines.size() - 2)
                    ciphertext += '\n';
            }

            CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
            CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);

            try {
                std::string decryptedtext;
                CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedtext));
                stfDecryptor.Put(reinterpret_cast<const unsigned char *>(ciphertext.c_str()), ciphertext.size());
                stfDecryptor.MessageEnd();

                if (decryptedtext.back() == '\0')
                    decryptedtext.pop_back();

                std::string decrypted_sha = this->hashString(decryptedtext);

                if (decrypted_sha == sha1) {
//                    std::cout << "[debug, Worker: " << this->id << "]: FOOUND for key: " << this->current_key << std::endl;

                    this->stop();

                    std::ofstream os(this->decrypted_file_path, std::ios::binary);
                    os << decryptedtext;
                    os.flush();
                    os.close();
                }else{
//                            boost::unique_lock<boost::mutex> lock(Worker::mu);
//                            std::cout << "[debug, Worker: " << this->id << "]: non except -> key: " << this->current_key << ", decrypted sha: " << decrypted_sha << " sha: " << sha1 << std::endl;
                }

            } catch (const CryptoPP::InvalidCiphertext& e) {
                // ...
//                        boost::unique_lock<boost::mutex> lock(Worker::mu);
//                        std::cout << "[debug, Worker: " << this->id << "]: exception for key: " << this->current_key << std::endl;
            }

            this->current_key += 1;

    }
}

