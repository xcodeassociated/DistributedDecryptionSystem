//
// Created by jm on 30.12.16.
//

#ifndef DDS_MASTERMAINCLASS_MOCK_HPP
#define DDS_MASTERMAINCLASS_MOCK_HPP

#include <gmock/gmock.h>
#include <gmock/gmock-actions.h>
#include <gmock/gmock-generated-actions.h>
#include <gmock/gmock-more-actions.h>

#include "Master.hpp"

namespace mock {
    class Master_mock : public Master {
        using Master::Master;

    public:
        Master_mock() : Master(nullptr) {
            ON_CALL(*this, foo()).
                    WillByDefault(testing::Return(true));
            
            ON_CALL(*this, foo2(testing::Matcher<int>(testing::_))).
                    WillByDefault(testing::Return(0));
            //...
        }
        
        MOCK_METHOD0(foo, bool());
        MOCK_METHOD0(fooint, int());
        MOCK_METHOD1(foo2, int(int x));
    };
    
}

#endif
