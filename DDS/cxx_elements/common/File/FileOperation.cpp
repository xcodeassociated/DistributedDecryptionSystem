//
// Created by Janusz Majchrzak on 14/09/2017.
//

#include <fstream>
#include <boost/filesystem.hpp>
#include "FileExceptions.hpp"

namespace File {

    void rename(const std::string& old_name, const std::string& new_name) {
        boost::filesystem::rename(old_name, new_name);
    }

    std::string get_file_path(const std::string& file) {
        boost::filesystem::path p(file);
        return p.parent_path().string();
    }

    std::string get_file_name(const std::string& file) {
        boost::filesystem::path p(file);
        return p.filename().string();
    }

    std::string make_path(const std::string& path, const std::string& file) {
        boost::filesystem::path file_object{file};
        boost::filesystem::path file_path{path};
        boost::filesystem::path new_path = file_path / file_object;
        return new_path.string();
    }
}