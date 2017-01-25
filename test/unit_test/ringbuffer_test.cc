//
// Created by fanziye on 17-1-25.
//

#include "gtest/gtest.h"
//#include "../../library.h"
#include "../../ringbuffer.hh"
#include <random>

class RingbufferTest: public testing::Test {

protected:
    typedef float data_t;
    data_t *source, *target;
    std::stringstream ss;
    virtual void SetUp() {
        source = new data_t[20]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
        target = new data_t[500];
    }
    virtual void TearDown() {
        delete []source;
        delete []target;
    }
};

TEST_F(RingbufferTest, enqueue_1) {
    Ringbuffer<data_t, 16> rb;
    bool result = rb.try_enqueue(source, 10);
    rb.print(std::cout);
    ASSERT_TRUE(result);
    result = rb.try_enqueue(source, 10);
    ASSERT_FALSE(result);
    rb.print(std::cout);
}

TEST_F(RingbufferTest, dequeue_1) {
    Ringbuffer<data_t, 16> rb;
    bool result = rb.try_enqueue(source, 10);
    rb.print(std::cout) << std::endl;
    ASSERT_TRUE(result);
    result = rb.try_enqueue(source, 10);
    ASSERT_FALSE(result);
    rb.print(std::cout) << std::endl;

    // dequeue 1
    result = rb.try_dequeue_advance(target, 3, 3);
    ASSERT_TRUE(result);
    rb.print(std::cout) << std::endl;

    // dequeue 2
    result = rb.try_dequeue_advance(target + 3, 3, 3);
    ASSERT_TRUE(result);
    rb.print(std::cout) << std::endl;

    // dequeue 3
    result = rb.try_dequeue_advance(target + 6, 8, 6);
    ASSERT_TRUE(result);
    rb.print(std::cout) << std::endl;

    // dequeue 4
    result = rb.try_dequeue_advance(target + 12, 8, 6);
    ASSERT_FALSE(result);
    rb.print(std::cout) << std::endl;
}