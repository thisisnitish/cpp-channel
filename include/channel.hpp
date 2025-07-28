#pragma once

#include <mutex>
#include <condition_variable>
#include <optional>

using namespace std;

template <typename T>
class Channel
{
public:
    explicit Channel(size_t buffer_size = 0); // buffer_size = 0 means unbuffered channel

    void send(const T &value);
    T receive();

private:
    mutex mtx;
    condition_variable cv_sender_;
    condition_variable cv_receiver_;

    // For buffered channels
    queue<T> buffer_;
    size_t buffer_size_;

    // For unbuffered channels, we use an optional to hold the data.
    optional<T> data_;
    bool has_data_ = false;
};

#include "channel.tpp"