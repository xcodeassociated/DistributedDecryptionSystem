//
// Created by Janusz Majchrzak on 17/08/2017.
//

#ifndef DDS_GATEWAYEXCEPTIONS_HPP
#define DDS_GATEWAYEXCEPTIONS_HPP

struct GatewayException : std::runtime_error {
    using std::runtime_error::runtime_error;
    GatewayException(const std::string& msg) : std::runtime_error{msg} {
        ;
    }
};

struct GatewaySendException : public GatewayException {
    GatewaySendException(const std::string& msg) : GatewayException{msg} {
        ;
    }
};

struct GatewayReceiveException : public GatewayException {
    GatewayReceiveException(const std::string& msg) : GatewayException{msg} {
        ;
    }
};

struct GatewayPingException : public GatewayException {
    GatewayPingException(const std::string& msg) : GatewayException{msg} {
        ;
    }
};

struct GatewayIncorrectRankException : public GatewayException {
    GatewayIncorrectRankException(const std::string& msg) : GatewayException{msg} {
        ;
    }
};

#endif //DDS_GATEWAYEXCEPTIONS_HPP
