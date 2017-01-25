//
// Created by fanziye on 17-1-21.
//

#include "gtest/gtest.h"
#include "../../library.h"
#include <algorithm>
#include <random>

TEST(pipeline_test, basic_usage_no_overlap) {
    std::vector<SeqPipeline::DataT> source_vec, result_vec;

    const std::size_t INPUT_COUNT(100);
    for (std::size_t i = 0; i < INPUT_COUNT; ++i) {
        source_vec.push_back(static_cast<SeqPipeline::DataT>(i));
    }
    for (std::size_t i = 0; i < source_vec.size(); ++i) {
        result_vec.push_back(source_vec[i]);
    }
    for (std::size_t i = 0; i < source_vec.size(); i += 2) {
        result_vec[i / 2] = result_vec[i] + result_vec[i + 1];
    }
    result_vec.erase(result_vec.begin() + result_vec.size() / 2, result_vec.end());
    for (std::size_t i = 0; i < source_vec.size(); i += 5) {
        result_vec[i / 5] = result_vec[i] + result_vec[i + 1]
                + result_vec[i + 2] + result_vec[i + 3] + result_vec[i + 4];
    }
    result_vec.erase(result_vec.begin() + result_vec.size() / 5, result_vec.end());

    std::vector<SeqPipeline::DataT> target_vec;

    SeqPipeline pipe;
    pipe.AddWork(std::make_shared<PlusOneWork>());
    pipe.AddWork(std::make_shared<PlusOneWork>());
    //pipe.AddWork(std::make_shared(PlusTonariTwo()));
    //pipe.AddWork(std::make_shared(MulTonariFive()));

    pipe.SetOutputCallback([&](std::shared_ptr<WorkingBuffer> data) {

        auto t = std::make_shared<WorkingBuffer>(20);
        data->dequeue_many(t);
        for (std::size_t idx = 0; idx < t->Count(); idx ++) {
            std::cout << t->m_buf.front() << " ";
            t->m_buf.pop();
        }
        std::cout << std::endl;
    });

    pipe.Start();
    std::thread input([&]() {
        std::cout << "Thread [writter]: Start" << std::endl;
        std::random_device rd;
        std::uniform_int_distribution<int> dist(1, 2);
        SeqPipeline::DataT* p_data = &(source_vec[0]);
        std::size_t amount = 0;
        std::size_t cur_amount = dist(rd);

        // FIXME: Cause of potential overflow
        cur_amount = amount + cur_amount > INPUT_COUNT ? INPUT_COUNT - amount : cur_amount;
        while (amount < INPUT_COUNT) {
            std::cout << "Thread [writter]: writing " << cur_amount << " value(s) "
                      << "with " << amount << " value(s) written" << std::endl;
            pipe.PushData(p_data + amount, cur_amount);
            amount += cur_amount;


            cur_amount = dist(rd);
            cur_amount = amount + cur_amount > INPUT_COUNT ? INPUT_COUNT - amount : cur_amount;
            sleep(0.1);
        }
        std::cout << "Thread [writter]: Quit" << std::endl;
    });
    input.join();
    //sleep(10);
    //pipe.Stop();
    pipe.Finish();
    sleep(2);
    //std::thread fini_thread(&SeqPipeline::Finish, std::ref(pipe));
    pipe.Stop();
    pipe.Join();

}