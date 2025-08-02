#pragma once

#include <condition_variable>
#include <future>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>

using namespace std;

template <typename T>
class Channel {
   public:
    explicit Channel(size_t buffer_size = 0);  // buffer_size = 0 means unbuffered channel

    void send(const T &value);  // Blocking send
    optional<T> receive();      // Blocking receive, returns nullopt if channel is closed and empty

    bool try_send(const T &value);  // Non-blocking send, returns false if channel is closed or full
    optional<T> try_receive();      // Non-blocking receive, returns nullopt if channel is closed and empty

    // Async Send: returns a future that resolves when the value is sent
    future<void> async_send(const T &value);
    // Async Receive: returns a future that resolves to the received value or nullopt if closed and empty
    future<optional<T>> async_receive();

    void close();            // Close the channel, no more sends allowed
    bool is_closed() const;  // Check if the channel is closed

    bool empty() const;

    // register a condition_variable to notify when channel state changes
    void add_notifier(std::condition_variable *cv) {
        std::lock_guard lock(mtx);
        notifiers_.push_back(cv);
    }

    bool is_receive_ready() {
        std::lock_guard<std::mutex> lock(mtx);
        if (buffer_size_ == 0) {
            return has_data_;
        } else {
            return !buffer_.empty();
        }
    }

   private:
    mutable mutex mtx;
    condition_variable cv_sender_;
    condition_variable cv_receiver_;

    // For buffered channels
    queue<T> buffer_;
    size_t buffer_size_;

    // For unbuffered channels, we use an optional to hold the data.
    optional<T> data_;
    bool has_data_ = false;

    bool closed_ = false;           // Indicates if the channel is closed
    size_t waiting_receivers_ = 0;  // Count of receivers waiting to receive

    std::vector<std::condition_variable *> notifiers_;  // subscribers (Selects etc.)

    void notify_all_registered() {
        for (auto cv : notifiers_) {
            cv->notify_all();
        }
    }
};

#include "channel.tpp"