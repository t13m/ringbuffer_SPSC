#ifndef PIPELINECPP_LIBRARY_H
#define PIPELINECPP_LIBRARY_H

#include <vector>
#include <memory>
#include <thread>
#include <functional>
#include <condition_variable>
#include <queue>
#include <cassert>
#include <iostream>

void hello();

class WorkingBuffer {
public:
    std::queue<unsigned int> m_buf;
    std::size_t m_output_amount;

    WorkingBuffer(std::size_t fixed_size) : m_output_amount(1) {}
    std::size_t Count() {
        return m_buf.size();
    }
    void SetOutputAmount(std::size_t amount) {m_output_amount = amount;}
    void enqueue_many(unsigned int * p_data, std::size_t amount) {
        for (std::size_t i = 0; i < amount; i ++) {
            m_buf.push(*(p_data + i));
        }
    }
    void dequeue_many(std::shared_ptr<WorkingBuffer> to) {
        for (std::size_t i = 0; i < m_output_amount && !m_buf.empty(); i ++) {
            to->m_buf.push(m_buf.front());
            m_buf.pop();
        }
    }

};

class APipelineWork {
public:
    APipelineWork(std::size_t input_bufsize, std::size_t output_bufsize, std::size_t require_input_amount) :
            m_input_bufsize(input_bufsize),
            m_output_bufsize(output_bufsize),
            m_require_input_amount(require_input_amount),
            m_output_amount(1),
            m_should_stop(true) {
        m_input_buffer = std::make_shared<WorkingBuffer>(m_input_bufsize);
        m_output_buffer = std::make_shared<WorkingBuffer>(m_output_bufsize);
    }
    virtual ~APipelineWork() {}
    virtual void Process() = 0;
    void Run() {
        std::cout << "Thread [Work.Run]: Start" << std::endl;
        m_should_stop = false;
        while (true) {
            std::cout << "Thread [Work.Run]: Trying to get lock" << std::endl;
            std::unique_lock<std::mutex> lk(m_mtx);
            if (!m_enough_data)
                m_cv_get_data.wait(lk, [&]() {
                    /*
                     * TODO [DONE]: Check the data is enough or should be quited
                     *
                     */
                    return m_should_stop || m_enough_data;
                });
            std::cout << "Thread [Work.Run]: Get the lock" << std::endl;
            if (m_should_stop) break;



            // Now that we have enough data
            // Do process now

            // Process is blocked
            this->Process();
            UpdateEnoughData();

            // Output
            m_output_next_cb(m_output_buffer);
        }
        std::cout << "Thread [Work.Run]: Quit" << std::endl;
    }
    void Stop() { m_should_stop = true; m_cv_get_data.notify_one();}
    bool IsStopped() const { return m_should_stop; }
    void SetOutputAmount(std::size_t output_amount) {
        m_output_buffer->SetOutputAmount(output_amount);
    }
    void SetOutputCallback(std::function<void(std::shared_ptr<WorkingBuffer>)> func) {
        m_output_next_cb = func;
    }
    bool ConnectTo(APipelineWork& feeder) {
        feeder.SetOutputAmount(m_require_input_amount);
        feeder.SetOutputCallback([this](std::shared_ptr<WorkingBuffer> output_buffer){this->OutputCallback(output_buffer);});
        return true;
    }

    // This function should called by the previous Work
    void OutputCallback(std::shared_ptr<WorkingBuffer> output_buffer) {
        output_buffer->dequeue_many(m_input_buffer);
        UpdateEnoughData();
        m_cv_get_data.notify_one();
    }

    void UpdateEnoughData() {
        std::lock_guard<std::mutex> lk(m_mtx_update);
        if (m_input_buffer->Count() >= m_require_input_amount)
            m_enough_data = true;
        else
            m_enough_data = false;
    }
protected:
    std::size_t m_input_bufsize, m_output_bufsize, m_require_input_amount, m_output_amount;
    std::shared_ptr<WorkingBuffer> m_input_buffer, m_output_buffer;
    bool m_should_stop;

    // should be used in the callback
    bool m_enough_data;
    std::condition_variable m_cv_get_data;
    std::mutex m_mtx, m_mtx_update;

    // callback functions
    std::function<void(std::shared_ptr<WorkingBuffer>)> m_output_next_cb;
};

/* POC */

class PlusOneWork: public APipelineWork {
public:
    PlusOneWork() :
        APipelineWork(10, 10, 1) {}
    virtual void Process() override {
        auto x = m_input_buffer->m_buf.front();
        m_input_buffer->m_buf.pop();
        m_output_buffer->m_buf.push(x + 1);
    }
};

class SeqPipeline {
public:
    typedef unsigned int DataT;

    SeqPipeline() {
        m_input_buffer = std::make_shared<WorkingBuffer>(20);
    };
    ~SeqPipeline() {};

    void Start() {
        for (auto rit = m_works.rbegin(); rit != m_works.rend(); rit ++) {
            m_threads.push_back(
                    std::make_shared<std::thread>(
                            [rit]{(*rit)->Run();}
                    ));
        }
    }
    void Stop() {
        for (auto it = m_works.begin(); it != m_works.end(); it ++) {
            (*it)->Stop();
        }
    }
    void Join() {
        for (auto rit = m_threads.rbegin(); rit != m_threads.rend(); rit ++) {
            (*rit)->join();
        }
    }
    void Finish() {
        while (m_input_buffer->Count() > 0) {
            m_works[0]->OutputCallback(m_input_buffer);
        }
    }
    bool IsStopped() const { return m_stopped; }
    void AddWork(std::shared_ptr<APipelineWork> p_work) {
        if (!m_works.empty())
            p_work->ConnectTo(*m_works.back());
        m_works.push_back(p_work);
    }
    void PushData(DataT *p_data, std::size_t amount) {
        m_input_buffer->enqueue_many(p_data, amount);
        m_works[0]->OutputCallback(m_input_buffer);
    }
    void SetOutputCallback(std::function<void(std::shared_ptr<WorkingBuffer>)> func) {
        assert(!m_works.empty());
        m_output_callback = func;
        m_works.back()->SetOutputCallback(m_output_callback);
    }


private:
    std::function<void(std::shared_ptr<WorkingBuffer>)> m_output_callback;
    bool m_stopped;
    std::vector<std::shared_ptr<APipelineWork>> m_works;
    std::vector<std::shared_ptr<std::thread>> m_threads;
    std::shared_ptr<WorkingBuffer> m_input_buffer, m_output_buffer;
};

#endif