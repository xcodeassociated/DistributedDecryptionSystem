//
// Created by jm on 30.12.16.
//

#include <gtest/gtest.h>

#include "Controller.hpp"

struct ControllerTests : testing::Test {
    core::Controller controller;
    
    ControllerTests() : controller{}{
        ;
    }
    
    ~ControllerTests() = default;
};

TEST_F(ControllerTests, Controller_foo_test){
    EXPECT_TRUE(controller.foo());
}

TEST_F(ControllerTests, Controller_inlinefoo_test){
    EXPECT_TRUE(controller.inline_foo());
}