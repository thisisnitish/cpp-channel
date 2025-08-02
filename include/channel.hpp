#pragma once

#include <condition_variable>
#include <future>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>

/**
 * @file channel.hpp
 * @brief Declaration of a Go-style channel in C++ supporting synchronous/asynchronous send/receive.
 *
 * @details
 * This Channel<T> implementation supports:
 *  - Buffered and unbuffered channels.
 *  - Blocking and non-blocking send/receive.
 *  - Async send/receive using std::future.
 *  - Close semantics (no more sends allowed).
 *  - Optional integration with Select<T> through notifier registration.
 *
 * @note Thread-safe: All public methods are safe for concurrent access
 *       from multiple producer and multiple consumer threads.
 *
 * @tparam T The type of messages passed through the channel.
 */

template <typename T>
class Channel {
   public:
    /**
     * @brief Constructs a Channel with optional buffering.
     * @param buffer_size Size of internal buffer. Set to 0 for unbuffered channel.
     */
    explicit Channel(std::size_t buffer_size = 0);

    /**
     * @brief Blocking send. Waits until the value is accepted by a receiver.
     * @param value The value to send.
     * @throws runtime_error if the channel is closed.
     */
    void send(const T &value);  // Blocking send

    /**
     * @brief Blocking receive. Waits for a value if the channel is not empty.
     * @return An optional value; std::nullopt if channel is closed and empty.
     */
    std::optional<T> receive();

    /**
     * @brief Non-blocking send.
     * @param value The value to send.
     * @return true if the value was accepted, false if channel is full or closed.
     */
    bool try_send(const T &value);

    /**
     * @brief Non-blocking receive.
     * @return An optional value if available, otherwise std::nullopt.
     */
    std::optional<T> try_receive();

    /**
     * @brief Asynchronously sends a value.
     * @param value The value to send.
     * @return A future that completes when the value is sent.
     */
    std::future<void> async_send(const T &value);

    /**
     * @brief Asynchronously receives a value.
     * @return A future that resolves to a received value or nullopt if closed and empty.
     */
    std::future<std::optional<T>> async_receive();

    /**
     * @brief Closes the channel. Further sends will fail.
     */
    void close();

    /**
     * @brief Checks if the channel is closed.
     * @return true if closed, false otherwise.
     */
    bool is_closed() const;

    /**
     * @brief Checks if the channel is empty.
     * @return true if empty or unbuffered with no pending data.
     */
    bool empty() const;

    /**
     * @brief Registers a condition_variable to notify when channel state changes.
     * Useful for implementing select-like functionality.
     */
    void add_notifier(std::condition_variable *cv) {
        std::lock_guard lock(mtx);
        notifiers_.push_back(cv);
    }

    /**
     * @brief Checks whether a receive operation can proceed immediately.
     * @return true if data is available, false otherwise.
     */
    bool is_receive_ready() {
        std::lock_guard<std::mutex> lock(mtx);
        if (buffer_size_ == 0) {  // unbuffered case
            return has_data_;
        } else {  // buffered case
            return !buffer_.empty();
        }
    }

   private:
    mutable std::mutex mtx;
    std::condition_variable cv_sender_;    // Notifies senders when space is available or data is consumed.
    std::condition_variable cv_receiver_;  // Notifies receivers when data is available.

    // For buffered channels
    std::queue<T> buffer_;
    std::size_t buffer_size_;  // 0 means unbuffered channel

    // For unbuffered channels, we use an optional to hold the data.
    std::optional<T> data_;
    bool has_data_ = false;

    bool closed_ = false;                // Indicates if the channel is closed
    std::size_t waiting_receivers_ = 0;  // Used to help with non-blocking send in unbuffered mode

    std::vector<std::condition_variable *> notifiers_;  // External notifiers for select-like coordination

    /**
     * @brief Notifies all registered condition variables (e.g., select implementations).
     */
    void notify_all_registered() {
        for (auto cv : notifiers_) {
            cv->notify_all();
        }
    }
};

#include "channel.tpp"