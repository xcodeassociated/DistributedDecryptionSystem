//
// Created by jm on 16.08.17.
//

#include "JsonFileOperations.hpp"
#include <rapidjson/rapidjson.h>
#include <rapidjson/document.h>

JsonFileOperations::JsonFileOperations(std::string _file_name) :
        file_name{_file_name} {
    ;
}