//
// Created by Janusz Majchrzak on 30/05/17.
//

#ifndef DDS_WORKERBASE_HPP
#define DDS_WORKERBASE_HPP


class WorkerBase {
public:
    virtual bool init() = 0;
    virtual ~WorkerBase() = default;
};


#endif //DDS_WORKERBASE_HPP
