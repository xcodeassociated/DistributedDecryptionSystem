//
// Created by Janusz Majchrzak on 17/08/2017.
//

#include "SlaveMessageHelper.hpp"
#include <Gateway.hpp>

MpiMessage SlaveMessageHelper::create_INFO_CALLBACK(int rank, const std::string& data, int response_message_id) {
    return {Gateway::id++, 0, rank, MpiMessage::Event::CALLBACK, false, data, MpiMessage::Callback{response_message_id, MpiMessage::Event::INFO}};
}

MpiMessage SlaveMessageHelper::create_INIT_CALLBACK(int rank, int response_message_id) {
    return {Gateway::id++, 0, rank, MpiMessage::Event::CALLBACK, false, "", MpiMessage::Callback{response_message_id, MpiMessage::Event::INIT}};
}

MpiMessage SlaveMessageHelper::create_PING_CALLBACK(int rank, const std::string& data, int response_message_id) {
    return {};
}

MpiMessage SlaveMessageHelper::create_FOUND(int rank, const std::string& data) {
    return {};
}