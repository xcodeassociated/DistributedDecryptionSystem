//
// Created by jm on 22.03.17.
//

#ifndef DDS_SLAVEMAINCLASS_HPP
#define DDS_SLAVEMAINCLASS_HPP

#include <MPIMessage.hpp>
#include <SyscomMessage.hpp>
#include <Logger.hpp>
#include <Decryptor.hpp>

namespace core {
    
    class Slave {
    public:
        bool init(void);
    };
    
}

#endif //DDS_SLAVEMAINCLASS_HPP
