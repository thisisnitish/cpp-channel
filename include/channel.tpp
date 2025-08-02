#pragma once

// Constructor
template <typename T>
Channel<T>::Channel(std::size_t buffer_size) : buffer_size_(buffer_size), has_data_(false), closed_(false) {}

// Send a value to the channel - Handles both buffered and unbuffered channels - Blocking Send
template <typename T>
void Channel<T>::send(const T &value) {
    std::unique_lock<std::mutex> lock(mtx);

    if (closed_) {
        throw std::runtime_error("Cannot send to a closed channel");
    }

    if (buffer_size_ == 0) {
        // Go with unbuffered channel logic

        // Wait if there's already data waiting to be received
        cv_sender_.wait(lock, [this]() { return !has_data_ || closed_; });

        data_ = value;
        has_data_ = true;

        cv_receiver_.notify_one();  // Notify a waiting receiver
        notify_all_registered();

        // Wait until receiver consumes it
        cv_sender_.wait(lock, [this]() { return !has_data_ || closed_; });
    } else {
        // Go with buffered channel logic
        cv_sender_.wait(lock, [this]() { return buffer_.size() < buffer_size_ || closed_; });

        if (closed_) {
            throw std::runtime_error("Cannot send to a closed channel");
        }

        buffer_.push(value);

        // Notify a waiting receiver that there's new data
        cv_receiver_.notify_one();
        notify_all_registered();
    }
}

// Receive a value from the channel - Handles both buffered and unbuffered channels - Blocking Receive
template <typename T>
std::optional<T> Channel<T>::receive() {
    std::unique_lock<std::mutex> lock(mtx);

    if (buffer_size_ == 0) {
        // Go with unbuffered channel logic

        waiting_receivers_++;
        // Wait until sender sends data
        cv_receiver_.wait(lock, [this]() { return has_data_ || closed_; });
        waiting_receivers_--;

        if (!has_data_ && closed_) {
            return std::nullopt;
        }

        T value = *data_;
        data_.reset();  // Clear the data after receiving
        has_data_ = false;

        // Notify sender that data has been consumed
        cv_sender_.notify_one();
        notify_all_registered();

        return value;
    } else {
        // Go with buffered channel logic

        // Wait until there's data in the buffer
        cv_receiver_.wait(lock, [this]() { return !buffer_.empty() || closed_; });

        if (buffer_.empty() && closed_) {
            return std::nullopt;
        }

        T value = buffer_.front();
        buffer_.pop();

        // Notify sender that there's space in the buffer
        cv_sender_.notify_one();
        notify_all_registered();
        return value;
    }
}

// Close the channel
template <typename T>
void Channel<T>::close() {
    std::unique_lock<std::mutex> lock(mtx);
    if (closed_)
        return;  // Already closed

    closed_ = true;

    // Notify all waiting threads
    cv_receiver_.notify_all();  // Notify all receivers that channel is closed
    cv_sender_.notify_all();    // Notify all senders that channel is closed
    notify_all_registered();
}

// Check closed state
template <typename T>
bool Channel<T>::is_closed() const {
    std::unique_lock<std::mutex> lock(mtx);
    return closed_;
}

// Check emptiness
template <typename T>
bool Channel<T>::empty() const {
    std::lock_guard<std::mutex> lock(mtx);
    if (buffer_size_ == 0) {
        return !has_data_;  // unbuffered behaviour
    } else {
        return buffer_.empty();  // buffered behaviour
    }
}

// Non-blocking Send
template <typename T>
bool Channel<T>::try_send(const T &value) {
    std::unique_lock<std::mutex> lock(mtx);

    if (closed_) return false;

    if (buffer_size_ == 0) {
        // unbuffered behavior, need a receiver to consume the data
        if (waiting_receivers_ == 0 || has_data_) return false;  // No receivers available
        data_ = value;
        has_data_ = true;
        cv_receiver_.notify_one();  // Notify a waiting receiver
        notify_all_registered();
        return true;

    } else {
        // buffered behavior
        if (buffer_.size() >= buffer_size_) return false;  // Buffer is full
        buffer_.push(value);
        cv_receiver_.notify_one();  // Notify a waiting receiver
        notify_all_registered();
        return true;
    }
}

// Non-blocking Receive
template <typename T>
std::optional<T> Channel<T>::try_receive() {
    std::unique_lock<std::mutex> lock(mtx);

    if (buffer_size_ == 0) {
        // unbuffered behavior
        if (!has_data_) return std::nullopt;  // No data available

        T value = *data_;
        data_.reset();  // Clear the data after receiving
        has_data_ = false;
        cv_sender_.notify_one();  // Notify sender that data has been consumed
        notify_all_registered();
        return value;
    } else {
        // buffered behavior
        if (buffer_.empty()) return std::nullopt;  // No data available
        T value = buffer_.front();
        buffer_.pop();
        cv_sender_.notify_one();  // Notify a waiting sender that there's space in the buffer
        notify_all_registered();
        return value;
    }
}

// Asynchronous Send using std::async
template <typename T>
std::future<void> Channel<T>::async_send(const T &value) {
    return std::async(std::launch::async, [this, value]() {
        this->send(value);
    });
}

// Asynchronous Receive using std::async
template <typename T>
std::future<std::optional<T>> Channel<T>::async_receive() {
    return std::async(std::launch::async, [this]() {
        return this->receive();
    });
}
