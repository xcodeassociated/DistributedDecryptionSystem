//
// Created by jm on 23.03.17.
//

#ifndef DDS_SLAVEMAINCLASS_MOCK_HPP
#define DDS_SLAVEMAINCLASS_MOCK_HPP

#include <gmock/gmock.h>
#include <gmock/gmock-actions.h>
#include <gmock/gmock-generated-actions.h>
#include <gmock/gmock-more-actions.h>

#include "SlaveMainClass.hpp"

namespace mock {
    class SlaveMainClass_mock : public core::SlaveMainClass {
    public:
        SlaveMainClass_mock() {
            ON_CALL(*this, foo()).
                    WillByDefault(testing::Return(true));
            
            ON_CALL(*this, foo2(testing::Matcher<int>(testing::_))).
                    WillByDefault(testing::Return(0));
            //...
        }
        
        MOCK_METHOD0(foo, bool());
        MOCK_METHOD0(fooint, int());
        MOCK_METHOD1(foo2, int(int x));
        
        MOCK_METHOD2(foo3, int(int x, int y));
    };
    
}

#endif //DDS_SLAVEMAINCLASS_MOCK_HPP
