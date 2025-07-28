#pragma once

template <typename T>
Channel<T>::Channel(size_t buffer_size) : buffer_size_(buffer_size), has_data_(false) {}

// Send a value to the channel - Handles both buffered and unbuffered channels
template <typename T>
void Channel<T>::send(const T &value)
{
    unique_lock<mutex> lock(mtx);

    if (buffer_size_ == 0)
    {
        // Go with unbuffered channel logic

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
    else
    {
        // Go with buffered channel logic
        cv_sender_.wait(lock, [this]()
                        { return buffer_.size() < buffer_size_; });

        buffer_.push(value);

        // Notify a waiting receiver that there's new data
        cv_receiver_.notify_one();
    }
}

// Receive a value from the channel - Handles both buffered and unbuffered channels
template <typename T>
T Channel<T>::receive()
{
    unique_lock<mutex> lock(mtx);

    if (buffer_size_ == 0)
    {
        // Go with unbuffered channel logic

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
    else
    {
        // Go with buffered channel logic

        // Wait until there's data in the buffer
        cv_receiver_.wait(lock, [this]()
                          { return !buffer_.empty(); });

        T value = buffer_.front();
        buffer_.pop();

        // Notify sender that there's space in the buffer
        cv_sender_.notify_one();
        return value;
    }
}
