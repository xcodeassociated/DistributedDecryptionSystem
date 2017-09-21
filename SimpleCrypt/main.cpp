#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <cryptopp/sha.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>

namespace po = boost::program_options;

void help_print(const po::options_description& desc){
    std::cout << desc;
    std::cout << std::endl;
    std::cout << "Example usage: " << std::endl;
    std::cout << "\tencryption: ./SimpleCrypt --key <decimal_value> --encrypt <file> --output <file>" << std::endl;
    std::cout << "\tdecryption: ./SimpleCrypt --key <decimal_value> --decrypt <file> --output <file>" << std::endl;
}

std::string hashFile(const boost::filesystem::path& file) {
    std::string result;
    CryptoPP::SHA1 sha1;
    CryptoPP::FileSource(file.string().c_str(), true,
                         new CryptoPP::HashFilter(sha1, new CryptoPP::HexEncoder(
                                 new CryptoPP::StringSink(result), true)));
    return result;
}

std::string hashString(const std::string& str){
    std::string result;
    CryptoPP::SHA1 sha1;
    CryptoPP::StringSource(str, true,
                           new CryptoPP::HashFilter(sha1, new CryptoPP::HexEncoder(
                                   new CryptoPP::StringSink(result), true)));
    return result;
}

std::vector<unsigned char> uint64ToBytes(uint64_t value) {
    std::vector<unsigned char> result(8, 0x00);
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

int verbose = 3;

int main(int argc, const char* argv[]) {

    try {
        po::options_description desc("SimpleCrypt Options");
        desc.add_options()
                ("help", "Prints SimpleCrypt help")
                ("sum", po::value<std::string>(), "Calculates SHA1 for a given file")
                ("encrypt", po::value<std::string>(), "Encrypts file")
                ("decrypt", po::value<std::string>(), "Decrypts file")
                ("output", po::value<std::string>(), "Output file")
                ("key", po::value<uint64_t>(), "AES Key file - int value")
                ("verbose", po::value<int>(),
                 "sets verbose: 1 = shows plain data form input, 2 = shows data for output, 0 = all");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            ::help_print(desc);
            return 0;
        }

        if (vm.count("verbose")) {
            verbose = vm["verbose"].as<int>();
        }

        if (vm.count("sum")) {
            std::string file_path = vm["sum"].as<std::string>();
            std::string hash = hashFile({file_path});

            std::cout << hash << std::endl;
            return 0;
        }

        if (vm.count("encrypt")) {
            if (!vm.count("key")) {
                std::cout << "missing: --key <key_value>" << std::endl;
                return 1;
            }

            std::string file_name = vm["encrypt"].as<std::string>();
            uint64_t key_int = vm["key"].as<uint64_t>();
            auto key_bytes = ::uint64ToBytes(key_int);

            assert(key_bytes.size() == CryptoPP::AES::DEFAULT_KEYLENGTH);
            byte *key = key_bytes.data();

            std::cout << "Hex KEY: ";
            int i = 0;
            for (; i < CryptoPP::AES::DEFAULT_KEYLENGTH;) {
                std::cout << "0x" << std::hex << (0xFF & static_cast<byte>(key[i++])) << " ";
            }
            std::cout << std::dec << std::endl;

            byte keyArray[CryptoPP::AES::BLOCKSIZE];
            memset(keyArray, 0x00, CryptoPP::AES::BLOCKSIZE);

            std::ifstream file_stream(file_name, std::ios::binary);
            if (!file_stream)
                throw std::runtime_error{"Cannot open source file!"};

            std::stringstream read_stream;
            read_stream << file_stream.rdbuf();
            std::string plaintext = read_stream.str();

            std::cout << "Plain Text size: " << plaintext.size() << " bytes" << std::endl;

            if (verbose == 0 || verbose == 1)
                std::cout << "Plain Text data: " << plaintext << std::endl;

            std::string sha1 = ::hashString(plaintext);
            std::cout << "Plain Text sha1: " << sha1 << std::endl;

            CryptoPP::AES::Encryption encryption_engine(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
            CryptoPP::CBC_Mode_ExternalCipher::Encryption cbc_mode_encryption(encryption_engine, keyArray);

            std::string encrypted_text = "";
            CryptoPP::StreamTransformationFilter st_filter(cbc_mode_encryption,
                                                           new CryptoPP::StringSink(encrypted_text));
            st_filter.Put(reinterpret_cast<const unsigned char *>(plaintext.c_str()), plaintext.length() + 1);
            st_filter.MessageEnd();

            std::cout << "Cipher Text size: " << encrypted_text.size() << " bytes" << std::endl;

            if (verbose == 0 || verbose == 2) {
                std::cout << "Cipher Text data: ";
                std::stringstream ss;
                for (std::size_t i = 0; i < encrypted_text.size(); i++) {
                    ss << "0x" << std::hex << (0xFF & static_cast<byte>(encrypted_text[i])) << " ";
                }
                std::cout << ss.str() << std::endl;
            }

            if (vm.count("output")) {
                std::string output = vm["output"].as<std::string>();
                std::cout << "Saving file: " << output << std::endl;

                std::ofstream os(output, std::ios::binary);
                if (!os)
                    throw std::runtime_error{"Cannot open output file"};

                os << sha1 << std::endl << encrypted_text << std::endl;
                os.flush();
                os.close();
            }

            return 0;
        }

        if (vm.count("decrypt")) {
            if (!vm.count("key")) {
                std::cout << "missing: --key <key_value>" << std::endl;
                return 1;
            }

            std::string file_name = vm["decrypt"].as<std::string>();
            uint64_t key_int = vm["key"].as<uint64_t>();
            auto key_bytes = ::uint64ToBytes(key_int);

            assert(key_bytes.size() == CryptoPP::AES::DEFAULT_KEYLENGTH);
            byte *key = key_bytes.data();

            std::cout << "Hex KEY: ";
            int i = 0;
            for (; i < CryptoPP::AES::DEFAULT_KEYLENGTH;) {
                std::cout << "0x" << std::hex << (0xFF & static_cast<byte>(key[i++])) << " ";
            }
            std::cout << std::dec << std::endl;

            byte keyArray[CryptoPP::AES::BLOCKSIZE];
            memset(keyArray, 0x00, CryptoPP::AES::BLOCKSIZE);

            std::ifstream file_stream(file_name, std::ios::binary);
            if (!file_stream)
                throw std::runtime_error{"Cannot open source file!"};

            std::stringstream read_stream;
            read_stream << file_stream.rdbuf();
            std::string read_stream_str = read_stream.str();

            std::vector<std::string> file_lines;
            boost::split(file_lines, read_stream_str, boost::is_any_of("\n"));

            std::string sha1 = file_lines[0];
            std::string ciphertext = "";
            for (std::size_t i = 1; i < file_lines.size(); i++) {
                ciphertext += file_lines[i];
                if (i < file_lines.size() - 2)
                    ciphertext += '\n';
            }

            std::cout << "sha1: " << sha1 << std::endl;

            if (verbose == 0 || verbose == 1)
                std::cout << "ciphertext: " << ciphertext << std::endl;

            CryptoPP::AES::Decryption decryption_engine(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
            CryptoPP::CBC_Mode_ExternalCipher::Decryption cbc_mode_decryption(decryption_engine, keyArray);

            try {
                std::string decrypted_text = "";
                CryptoPP::StreamTransformationFilter st_filter(cbc_mode_decryption,
                                                               new CryptoPP::StringSink(decrypted_text));
                st_filter.Put(reinterpret_cast<const unsigned char *>(ciphertext.c_str()), ciphertext.size());
                st_filter.MessageEnd();

                if (decrypted_text.back() == '\0')
                    decrypted_text.pop_back();

                std::cout << "Decrypted size: " << decrypted_text.size() << std::endl;

                if (verbose == 0 || verbose == 2)
                    std::cout << "Decrypted: " << decrypted_text << std::endl;

                std::string decrypted_sha = ::hashString(decrypted_text);
                std::cout << "decrypted sha1: " << decrypted_sha << std::endl;

                if (decrypted_sha == sha1)
                    std::cout << "checksum match!" << std::endl;
                else
                    std::cout << "checksum DOESN'T match!" << std::endl;

                if (vm.count("output")) {
                    std::string output = vm["output"].as<std::string>();
                    std::cout << "Saving file: " << output << std::endl;
                    std::ofstream os(output, std::ios::binary);
                    if (!os)
                        throw std::runtime_error{"Cannot open output file"};

                    os << decrypted_text;
                    os.flush();
                    os.close();
                }

            } catch (const CryptoPP::InvalidCiphertext &e) {
                std::cerr << "CryptoPP::InvalidCiphertext Exception: " << e.what() << std::endl;
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
        }
    } catch (const po::error& e) {
        std::cerr << "ProgramOptions Exception: " << e.what();
        return EXIT_FAILURE;
    } catch (const std::runtime_error& er) {
        std::cerr << "Exception: " << er.what();
        return  EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}