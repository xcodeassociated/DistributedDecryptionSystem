//
// Created by Janusz Majchrzak on 14/09/2017.
//

#ifndef DDS_FILEEXCEPTIONS_HPP
#define DDS_FILEEXCEPTIONS_HPP

#include <string>
#include <stdexcept>

struct FileException : public std::runtime_error {
    using std::runtime_error::runtime_error;
    FileException(const std::string& desc) : std::runtime_error{desc} { ; }
};

struct FileNotAccessibleException : public FileException {
    FileNotAccessibleException(const std::string& desc) : FileException{desc} { ; }
};

struct FileEmptyException : public FileException {
    FileEmptyException(const std::string& desc) : FileException{desc} { ; }
};

#endif //DDS_FILEEXCEPTIONS_HPP
