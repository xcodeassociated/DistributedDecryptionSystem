//
// Created by Janusz Majchrzak on 30/05/17.
//

#ifndef DDS_WORKERBASE_HPP
#define DDS_WORKERBASE_HPP

#include <cstdint>
#include <string>
#include <boost/atomic.hpp>
#include <SysCom.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lockfree/spsc_queue.hpp>

struct KeyRange{
    uint64_t begin;
    uint64_t end;
};

class WorkerBase {
protected:
    using SysComSTR = SysCom<std::string>;

    int id = 0;
    bool work = false;
    bool found = false;
    KeyRange range{0, 0};
    boost::atomic<uint64_t> current_key{0};
    boost::shared_ptr<boost::lockfree::spsc_queue<SysComSTR>> rx = nullptr;
    boost::shared_ptr<boost::lockfree::spsc_queue<SysComSTR>> tx = nullptr;

public:

    WorkerBase(boost::shared_ptr<boost::lockfree::spsc_queue<SysComSTR>>, boost::shared_ptr<boost::lockfree::spsc_queue<SysComSTR>>);

    virtual void worker_process() = 0;

    virtual void start();

    virtual void stop();

    virtual void process_syscom() = 0;

    virtual ~WorkerBase() = default;

    virtual uint64_t get_current_key() {
        return this->current_key;
    }
};


#endif //DDS_WORKERBASE_HPP
