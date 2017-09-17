//
// Created by Janusz Majchrzak on 01/06/17.
//

#ifndef DDS_MPIMESSAGE_HPP
#define DDS_MPIMESSAGE_HPP

#include <boost/serialization/string.hpp>
#include <boost/serialization/optional.hpp>

using data_type = std::string;
using rank_type = int;
using message_id_type = int;

struct MpiMessage{
    enum class Event : int {
        CALLBACK = 0,
        INIT = 1,
        PING = 2,
        FOUND = 3,
        KILL = 4,
        INFO = 5
    };

    struct Callback{
        message_id_type message_id;
        Event event;

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version) {
            (void)version;
            ar & message_id;
            ar & event;
        }
    };

    message_id_type id;
    rank_type receiver;
    rank_type sender;
    Event event;
    bool need_respond = false;
    data_type data;
    // only for CALLBACK message
    boost::optional<Callback> respond_to;

    MpiMessage() = default;
    MpiMessage(message_id_type _id,
               rank_type _receiver,
               rank_type _sender,
               Event _event,
               bool _need_respond,
               data_type data,
               boost::optional<Callback> res = boost::none)
            : id{_id},
              receiver{_receiver},
              sender{_sender},
              event{_event},
              need_respond{_need_respond},
              data{data},
              respond_to{res}
    {}

    MpiMessage(const MpiMessage&) = default;

    friend class boost::serialization::access;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version) {
        (void)version;
        ar & id;
        ar & receiver;
        ar & sender;
        ar & event;
        ar & need_respond;
        ar & data;
        ar & respond_to;
    }
};

#endif //DDS_MPIMESSAGE_HPP
