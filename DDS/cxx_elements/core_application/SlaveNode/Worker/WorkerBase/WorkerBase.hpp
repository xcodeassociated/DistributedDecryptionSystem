//
// Created by Janusz Majchrzak on 30/05/17.
//

#ifndef DDS_WORKERBASE_HPP
#define DDS_WORKERBASE_HPP

#include <cstdint>
#include <boost/atomic.hpp>

struct KeyRange{
    uint64_t begin;
    uint64_t end;
};

class WorkerBase {
protected:
    int id = 0;
    bool work = false;
    KeyRange range{0, 0};
    boost::atomic<uint64_t> current_key{0};

public:

    virtual void worker_process() = 0;

    virtual void start();

    virtual void stop();

    virtual ~WorkerBase() = default;
};


#endif //DDS_WORKERBASE_HPP
