//
// Created by Janusz Majchrzak on 30/05/17.
//

#ifndef DDS_WORKERIMPLEMENTATION_HPP
#define DDS_WORKERIMPLEMENTATION_HPP

#include <WorkerBase.hpp>

class Decryptor : public WorkerBase {
public:
    virtual bool init() override ;
    Decryptor();
};


#endif //DDS_WORKERIMPLEMENTATION_HPP
