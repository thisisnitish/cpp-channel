#pragma once

template <typename T>
Channel<T>::Channel(size_t buffer_size) : buffer_size_(buffer_size), has_data_(false), closed_(false) {}

// Send a value to the channel - Handles both buffered and unbuffered channels
template <typename T>
void Channel<T>::send(const T &value) {
    unique_lock<mutex> lock(mtx);

    if (closed_) {
        throw runtime_error("Cannot send to a closed channel");
    }

    if (buffer_size_ == 0) {
        // Go with unbuffered channel logic

        // Wait if there's already data waiting to be received
        cv_sender_.wait(lock, [this]() { return !has_data_ || closed_; });

        data_ = value;
        has_data_ = true;

        cv_receiver_.notify_one();  // Notify a waiting receiver

        // Wait until receiver consumes it
        cv_sender_.wait(lock, [this]() { return !has_data_ || closed_; });
    } else {
        // Go with buffered channel logic
        cv_sender_.wait(lock, [this]() { return buffer_.size() < buffer_size_ || closed_; });

        if (closed_) {
            throw runtime_error("Cannot send to a closed channel");
        }

        buffer_.push(value);

        // Notify a waiting receiver that there's new data
        cv_receiver_.notify_one();
    }
}

// Receive a value from the channel - Handles both buffered and unbuffered channels
template <typename T>
optional<T> Channel<T>::receive() {
    unique_lock<mutex> lock(mtx);

    if (buffer_size_ == 0) {
        // Go with unbuffered channel logic

        waiting_receivers_++;
        // Wait until sender sends data
        cv_receiver_.wait(lock, [this]() { return has_data_ || closed_; });
        waiting_receivers_--;

        if (!has_data_ && closed_) {
            return nullopt;
        }

        T value = *data_;
        data_.reset();  // Clear the data after receiving
        has_data_ = false;

        // Notify sender that data has been consumed
        cv_sender_.notify_one();

        return value;
    } else {
        // Go with buffered channel logic

        // Wait until there's data in the buffer
        cv_receiver_.wait(lock, [this]() { return !buffer_.empty() || closed_; });

        if (buffer_.empty() && closed_) {
            return nullopt;
        }

        T value = buffer_.front();
        buffer_.pop();

        // Notify sender that there's space in the buffer
        cv_sender_.notify_one();
        return value;
    }
}

template <typename T>
void Channel<T>::close() {
    unique_lock<mutex> lock(mtx);
    if (closed_)
        return;  // Already closed

    closed_ = true;
    cv_receiver_.notify_all();  // Notify all receivers that channel is closed
    cv_sender_.notify_all();    // Notify all senders that channel is closed
}

template <typename T>
bool Channel<T>::is_closed() const {
    unique_lock<mutex> lock(mtx);
    return closed_;
}

template <typename T>
bool Channel<T>::try_send(const T &value) {
    unique_lock<mutex> lock(mtx);

    if (closed_) return false;

    if (buffer_size_ == 0) {
        // unbuffered behavior, need a receiver to consume the data
        if (waiting_receivers_ == 0 || has_data_) return false;  // No receivers available
        data_ = value;
        has_data_ = true;
        cv_receiver_.notify_one();  // Notify a waiting receiver
        return true;

    } else {
        // buffered behavior
        if (buffer_.size() >= buffer_size_) return false;  // Buffer is full
        buffer_.push(value);
        cv_receiver_.notify_one();  // Notify a waiting receiver
        return true;
    }
}

template <typename T>
optional<T> Channel<T>::try_receive() {
    unique_lock<mutex> lock(mtx);

    if (buffer_size_ == 0) {
        // unbuffered behavior
        if (!has_data_) return nullopt;  // No data available

        T value = *data_;
        data_.reset();  // Clear the data after receiving
        has_data_ = false;
        cv_sender_.notify_one();  // Notify sender that data has been consumed
        return value;
    } else {
        // buffered behavior
        if (buffer_.empty()) return nullopt;  // No data available
        T value = buffer_.front();
        buffer_.pop();
        cv_sender_.notify_one();  // Notify a waiting sender that there's space in the buffer
        return value;
    }
}
