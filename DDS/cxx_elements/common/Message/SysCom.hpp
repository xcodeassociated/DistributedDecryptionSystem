//
// Created by Janusz Majchrzak on 21/08/2017.
//

#ifndef DDS_SYSCOM_HPP
#define DDS_SYSCOM_HPP

#include <cstdint>

template <typename T>
struct SysCom {
    enum class Type : int {
        PING = 1,
        KILL,
        FOUND,
        CALLBACK
    };

    T data;
    Type type;
};

#endif //DDS_SYSCOM_HPP
