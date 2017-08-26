#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <cryptopp/sha.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/hex.h>
#include <cryptopp/files.h>

namespace po = boost::program_options;

void help_print(){
    std::cout << "Help..." << std::endl;
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

int main(int argc, const char* argv[]) {

    po::options_description desc("SimpleCrypt Options");
    desc.add_options()
            ("help", "Prints SimpleCrypt help")
            ("sum", po::value<std::string>(), "Calculates SHA1 for a given file")
            ("encrypt", po::value<std::string>(), "Encrypts file")
            ("decrypt", po::value<std::string>(), "Decrypts file")
            ("output", po::value<std::string>(), "Output file")
            ("key", po::value<uint64_t>(), "AES Key file - int value");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        ::help_print();
        return 0;
    }

    if (vm.count("sum")) {
        std::string file_path = vm["sum"].as<std::string>();
        std::string hash = hashFile({file_path});

        std::cout << hash << std::endl;
        return 0;
    }

    if (vm.count("encrypt")){
        if (!vm.count("key")){
            std::cout << "missing: --key <key_value>" << std::endl;
            return 1;
        }

        std::string file_name = vm["encrypt"].as<std::string>();
        uint64_t key_int = vm["key"].as<uint64_t>();
        auto key_bytes = ::uint64ToBytes(key_int);

        assert(key_bytes.size() == CryptoPP::AES::DEFAULT_KEYLENGTH);
        byte* key = key_bytes.data();

        std::cout << "Hex KEY: ";
        int i = 0;
        for (; i < CryptoPP::AES::DEFAULT_KEYLENGTH; ) {
            std::cout << "0x" << std::hex << (0xFF & static_cast<byte>(key[i++])) << " ";
        }
        std::cout << std::dec << std::endl;

        byte iv[ CryptoPP::AES::BLOCKSIZE ];
        memset( iv, 0x00, CryptoPP::AES::BLOCKSIZE );

        std::ifstream file_stream(file_name, std::ios::binary);
        std::stringstream read_stream;
        read_stream << file_stream.rdbuf();
        std::string plaintext = read_stream.str();

        std::cout << "Plain Text size: " << plaintext.size() << " bytes" << std::endl;
        std::cout << "Plain Text data: " << plaintext << std::endl;

        std::string sha1 = ::hashString(plaintext);
        std::cout << "Plain Text sha1: " << sha1 << std::endl;

        CryptoPP::AES::Encryption aesEncryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
        CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption( aesEncryption, iv );

        std::string ciphertext;
        CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink( ciphertext ) );
        stfEncryptor.Put( reinterpret_cast<const unsigned char*>( plaintext.c_str() ), plaintext.length() + 1 );
        stfEncryptor.MessageEnd();

        std::cout << "Cipher Text size: " << ciphertext.size() << " bytes" << std::endl;
        std::cout << "Cipher Text data: ";
        std::stringstream ss;
        for( int i = 0; i < ciphertext.size(); i++ ) {
            ss << "0x" << std::hex << (0xFF & static_cast<byte>(ciphertext[i])) << " ";
        }
        std::cout << ss.str() << std::endl;

        if (vm.count("output")){
            std::string output = vm["output"].as<std::string>();
            std::cout << "Saving file: " << output << std::endl;
            std::ofstream os(output, std::ios::binary);
            os << sha1 << std::endl << ciphertext << std::endl;
            os.flush();
            os.close();
        }

        return 0;
    }

    if (vm.count("decrypt")){
        if (!vm.count("key")){
            std::cout << "missing: --key <key_value>" << std::endl;
            return 1;
        }

        std::string file_name = vm["decrypt"].as<std::string>();
        uint64_t key_int = vm["key"].as<uint64_t>();
        auto key_bytes = ::uint64ToBytes(key_int);

        assert(key_bytes.size() == CryptoPP::AES::DEFAULT_KEYLENGTH);
        byte* key = key_bytes.data();

        std::cout << "Hex KEY: ";
        int i = 0;
        for (; i < CryptoPP::AES::DEFAULT_KEYLENGTH; ) {
            std::cout << "0x" << std::hex << (0xFF & static_cast<byte>(key[i++])) << " ";
        }
        std::cout << std::dec <<std::endl;

        byte iv[CryptoPP::AES::BLOCKSIZE];
        memset(iv, 0x00, CryptoPP::AES::BLOCKSIZE);

        std::ifstream file_stream(file_name, std::ios::binary);
        std::stringstream read_stream;
        read_stream << file_stream.rdbuf();
        std::string read_stream_str = read_stream.str();

        std::vector<std::string> file_lines;
        boost::split(file_lines, read_stream_str, boost::is_any_of("\n"));

        std::string sha1 = file_lines[0];
        std::string ciphertext = "";
        for (int i = 1; i < file_lines.size(); i++) {
            ciphertext += file_lines[i];
            if (i < file_lines.size() - 2)
                ciphertext += '\n';
        }

        std::cout << "sha1: " << sha1 << std::endl;
        std::cout << "ciphertext: " << ciphertext << std::endl;

        CryptoPP::AES::Decryption aesDecryption(key, CryptoPP::AES::DEFAULT_KEYLENGTH);
        CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv);

        try {
            std::string decryptedtext;
            CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedtext));
            stfDecryptor.Put(reinterpret_cast<const unsigned char *>( ciphertext.c_str()), ciphertext.size());
            stfDecryptor.MessageEnd();

            if (decryptedtext.back() == '\0')
                decryptedtext.pop_back();

            std::cout << "Decrypted size: " << decryptedtext.size() << std::endl;

            std::cout << "Decrypted: " << decryptedtext << std::endl;
            std::string decrypted_sha = ::hashString(decryptedtext);
            std::cout << "decrypted sha1: " << decrypted_sha << std::endl;

            if (decrypted_sha == sha1)
                std::cout << "checksum match!" << std::endl;
            else
                std::cout << "checksum DOESN'T match!" << std::endl;

            if (vm.count("output")) {
                std::string output = vm["output"].as<std::string>();
                std::cout << "Saving file: " << output << std::endl;
                std::ofstream os(output, std::ios::binary);
                os << decryptedtext;
                os.flush();
                os.close();
            }

        } catch (const CryptoPP::InvalidCiphertext &e) {
            std::cout << "CryptoPP::InvalidCiphertext Exception: " << e.what() << std::endl;
            return 1;
        }

        return 0;
    }

    ::help_print();
    return 0;
}