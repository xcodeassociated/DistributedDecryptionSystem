//
// Created by jm on 23.03.17.
//

#ifndef DDS_SLAVEMAINCLASS_MOCK_HPP
#define DDS_SLAVEMAINCLASS_MOCK_HPP

#include <gmock/gmock.h>
#include <gmock/gmock-actions.h>
#include <gmock/gmock-generated-actions.h>
#include <gmock/gmock-more-actions.h>

#include "Slave.hpp"

namespace mock {
    class Slave_mock : public Slave {
    public:
        Slave_mock() {
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

#endif //DDS_SLAVEMAINCLASS_MOCK_HPP
