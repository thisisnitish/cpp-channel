#pragma once

template <typename T>
Channel<T>::Channel() : has_data_(false) {}

template <typename T>
void Channel<T>::send(const T &value)
{
    unique_lock<mutex> lock(mtx);

    // Wait if there's already data waiting to be received
    cv_sender_.wait(lock, [this]()
                    { return !has_data_; });

    data_ = value;
    has_data_ = true;

    cv_receiver_.notify_one(); // Notify a waiting receiver

    // Wait until receiver consumes it
    cv_sender_.wait(lock, [this]()
                    { return !has_data_; });
}

template <typename T>
T Channel<T>::receive()
{
    unique_lock<mutex> lock(mtx);

    // Wait until sender sends data
    cv_receiver_.wait(lock, [this]()
                      { return has_data_; });

    T value = *data_;
    data_.reset(); // Clear the data after receiving
    has_data_ = false;

    // Notify sender that data has been consumed
    cv_sender_.notify_one();

    return value;
}
