//
// Created by Janusz Majchrzak on 30/05/17.
//

#include <stdexcept>
#include "WorkerBase.hpp"

WorkerBase::WorkerBase(boost::shared_ptr<boost::lockfree::spsc_queue<SysComSTR>> _rx,
                       boost::shared_ptr<boost::lockfree::spsc_queue<SysComSTR>> _tx) : rx{_rx}, tx{_tx} { ; }

void WorkerBase::start(){
    if (!this->work) {
        this->worker_process();
    } else
        throw std::runtime_error{"Worker already started! 0x0004"};
}

void WorkerBase::stop(){
    this->work = false;
}