//
// Created by Janusz Majchrzak on 16/08/2017.
//

#include "MasterMessageHelper.hpp"
#include <sstream>
#include <string>
#include <Gateway.hpp>

MpiMessage MessageHelper::create_INFO(int rank) {
    return {Gateway::id++, rank, 0, MpiMessage::Event::INFO, true, "?"};
}

MpiMessage MessageHelper::create_INIT(int rank, const std::string& data) {
    return {Gateway::id++, rank, 0, MpiMessage::Event::INIT, true, data};
}

MpiMessage MessageHelper::create_PING(int rank) {
    return {Gateway::id++, rank, 0, MpiMessage::Event::PING, true, "?"};
}

MpiMessage MessageHelper::create_KILL(int rank) {
    return {Gateway::id++, rank, 0, MpiMessage::Event::KILL, false, "kill"};
}
