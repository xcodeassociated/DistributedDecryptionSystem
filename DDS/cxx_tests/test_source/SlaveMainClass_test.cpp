//
// Created by jm on 30.12.16.
//

#include <gtest/gtest.h>

#include "SlaveMainClass_mock.hpp"

struct SlaveMainClass_test : testing::Test {
    mock::SlaveMainClass_mock model;
    
    SlaveMainClass_test() : model{}{
        ;
    }
    
    ~SlaveMainClass_test() = default;
};

TEST_F(SlaveMainClass_test, SlaveMainClass_test_foo_test){
    // not needed since mock class has a default returns set up, but without it we get gmock warnings
    // (same to all the others that has default return defined in a mock class)
    EXPECT_CALL(model, foo())
            .WillOnce(testing::Return(true));
    
    EXPECT_TRUE(model.foo());
}

TEST_F(SlaveMainClass_test, SlaveMainClass_test_fooint_test){
    EXPECT_CALL(model, fooint())
            .WillOnce(testing::Return(0));
    
    EXPECT_EQ(model.fooint(), 0);
}

TEST_F(SlaveMainClass_test, SlaveMainClass_test_foo2_test){
    EXPECT_CALL(model, foo2(testing::Matcher<int>(10)))
            .WillRepeatedly(testing::Return(0));
    
    EXPECT_EQ(model.foo2(10), 0);
    //EXPECT_EQ(model.foo2(9999), 0); // <- gmock error: arg is not equal to 10
}

TEST_F(SlaveMainClass_test, SlaveMainClass_test_foo2_test2){
    EXPECT_CALL(model, foo2(testing::Matcher<int>(testing::_)))
            .WillRepeatedly(testing::Return(0));
    
    EXPECT_EQ(model.foo2(10), 0);
    EXPECT_EQ(model.foo2(9999), 0);
    EXPECT_EQ(model.foo2(0), 0);
}

TEST_F(SlaveMainClass_test, SlaveMainClass_test_foo2_test3){
    EXPECT_CALL(model, foo2(testing::Matcher<int>(testing::_)))
            .WillOnce(testing::Return(0));
    
    EXPECT_EQ(model.foo2(10), 0);
    //EXPECT_EQ(model.foo2(0), 0); //<- gtest error: expected to be called once
}

TEST_F(SlaveMainClass_test, SlaveMainClass_test_foo3_test0){
    EXPECT_CALL(model, foo3(testing::Matcher<int>(testing::_), testing::Matcher<int>(testing::_)))
            .WillOnce(testing::Return(100));
    
    EXPECT_EQ(model.foo3(0, 1), 100);
}