//
// Created by jm on 30.12.16.
//

#ifndef SLAVE_NODE_MODELMOCK_HPP
#define SLAVE_NODE_MODELMOCK_HPP

#include <gmock/gmock.h>
#include <gmock/gmock-actions.h>
#include <gmock/gmock-generated-actions.h>
#include <gmock/gmock-more-actions.h>

#include "Model.hpp"

namespace mock {
    class ModelMock : public core::Model {
    public:
        ModelMock() {
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
#endif //SLAVE_NODE_MODELMOCK_HPP
