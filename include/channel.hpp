#pragma once

#include <mutex>
#include <condition_variable>
#include <optional>

using namespace std;

template <typename T>
class Channel
{
public:
    Channel();

    void send(const T &value);
    T receive();

private:
    mutex mtx;
    condition_variable cv_sender_;
    condition_variable cv_receiver_;

    optional<T> data_;
    bool has_data_ = false;
};

#include "channel.tpp"