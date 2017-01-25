//
// Created by fanziye on 17-1-24.
//

#ifndef PIPELINECPP_RINGBUFFER_HH
#define PIPELINECPP_RINGBUFFER_HH

#include <cstddef>
#include <cassert>
#include <algorithm>
#include <atomic>
#include <mutex>

template<typename T, std::size_t CAPACITY>
class Ringbuffer {
    static_assert (CAPACITY > 0, "empty ringbuffer is not allowed");

public:
    Ringbuffer() : m_read_pos(0), m_write_pos(0), m_buf(nullptr) {
        m_buf = new T[CAPACITY];
    }
    ~Ringbuffer() {
        if (m_buf != nullptr) {
            delete [] m_buf;
        }
    }

    std::size_t count() const {
        return m_write_pos - m_read_pos;
    }

    bool empty() const {
        return ((m_write_pos == m_read_pos) || (m_read_pos > m_write_pos && (m_write_pos - m_read_pos) > CAPACITY - 1));
    }

    bool full() const {
        return m_write_pos - m_read_pos >= CAPACITY;
    }

    void clear() {
        m_read_pos = 0;
        m_write_pos = 0;
    }

    bool try_enqueue(const T *from, const std::size_t amount) {
        assert(amount <= CAPACITY);

        std::unique_lock<std::mutex> lk(m_mtx, std::defer_lock);

        bool overwriten = amount > CAPACITY - count();
        if (overwriten) {
            lk.lock();

            // Calculate again, in case that there is enough space now.
            overwriten = amount > CAPACITY - count();
        }

        std::size_t idx_write_pos_new = m_write_pos + amount;
        if (m_write_pos % CAPACITY < idx_write_pos_new % CAPACITY) {
            std::copy(from, from + amount, m_buf + m_write_pos % CAPACITY);
        } else {
            std::size_t n_first_write = CAPACITY - m_write_pos % CAPACITY;
            std::copy(from, from + n_first_write, m_buf + m_write_pos % CAPACITY);

            std::size_t n_rest_write = amount - n_first_write;
            std::copy(from + n_first_write, from + amount, m_buf);
        }
        m_write_pos = idx_write_pos_new;
        if (count() > CAPACITY) {
            m_read_pos += m_write_pos - CAPACITY;
        }
        return !overwriten;
    }

    bool try_dequeue_advance(T *to, const std::size_t amount, const std::size_t n_advance) {
        assert(amount <= CAPACITY && n_advance <= CAPACITY);
        std::lock_guard<std::mutex> lk(m_mtx);
        if (amount > count()) {
            // data not enough
            return false;
        }

        std::size_t idx_head_new = m_read_pos + amount;
        if (m_read_pos % CAPACITY < idx_head_new % CAPACITY) {
            std::copy(m_buf + m_read_pos % CAPACITY, m_buf + m_read_pos % CAPACITY + amount, to);
        } else {
            std::size_t n_first_read = CAPACITY - m_read_pos % CAPACITY;
            std::copy(m_buf + m_read_pos % CAPACITY, m_buf + m_read_pos % CAPACITY + n_first_read, to);

            std::size_t n_rest_read = amount - n_first_read;
            std::copy(m_buf, m_buf + n_rest_read, to + n_first_read);
        }
        m_read_pos += n_advance;
        return true;
    }

    // Copy to T* to, and return copied amount.
    std::size_t dequeue_as_many(T *to, const std::size_t amount) {
        std::lock_guard<std::mutex> lk(m_mtx);

        std::size_t n_read = amount <= count() ? amount : count();
        std::size_t idx_head_new = m_read_pos + n_read;
        if (m_read_pos % CAPACITY < idx_head_new % CAPACITY) {
            std::copy(m_buf + m_read_pos % CAPACITY, m_buf + m_read_pos % CAPACITY + n_read, to);
        } else {
            std::size_t n_first_read = CAPACITY - m_read_pos % CAPACITY;
            std::copy(m_buf + m_read_pos % CAPACITY, m_buf + m_read_pos % CAPACITY + n_first_read, to);

            std::size_t n_rest_read = n_read - n_first_read;
            std::copy(m_buf, m_buf + n_rest_read, to + n_first_read);
        }
        m_read_pos += n_read;
        return n_read;
    }

    std::ostream& print(std::ostream& os) const {
        for (std::size_t i = m_read_pos; i < m_write_pos; ++ i) {
            os << m_buf[i % CAPACITY];
            if (i + 1 < m_write_pos)
                os << " ";
        }
        return os;
    }

private:
    std::atomic_uint_fast32_t m_read_pos, m_write_pos;
    T *m_buf;
    std::mutex m_mtx;
};

#endif //PIPELINECPP_RINGBUFFER_HH