#pragma once

#include <cassert>
#include <iostream>
#include <optional>
#include <random>
#include <vector>

#include "channel.hpp"
using namespace std;

/**
 * @file select.hpp
 * @brief Declaration of a Go-style select mechanism for multiple channels.
 *
 * @details
 * Select<T> allows waiting on multiple channel operations (send/receive) and optionally a default case.
 * It integrates with Channel<T> through notifier registration to support blocking waits.
 *
 * Behaviour:
 *  - At most one ready case is executed per run/run_blocking call.
 *  - If multiple cases are ready, one is chosen at random (no fairness guarantee).
 *  - Default case runs immediately if no other case is ready.
 *  - run_blocking() blocks until any case is ready, cancelled, or timeout expires.
 *
 * @tparam T The channel message type.
 */

template <typename T>
class Select {
   public:
    /**
     * @brief Add a receive case to the selector.
     * @param chan The channel to receive from.
     * @return Reference to the `Select` object specially for chaining.
     */
    Select& receive(Channel<T>& chan);

    /**
     * @brief Add a send case to the selector.
     * @param chan The channel to send to.
     * @param val The value to send.
     * @return Reference to the `Select` object for chaining.
     */
    Select& send(Channel<T>& chan, const T& val);

    /**
     * @brief Adds a default (fallback) case to the selector.
     * This will be chosen if no other case is ready.
     */
    Select& default_case();

    /**
     * @brief Executes a non-blocking probe over all cases.
     *
     * @return true if any send or receive case succeeded or default case triggered.
     */
    bool run();

    /**
     * @brief Blocking version of run(). Waits until one case becomes ready.
     *
     * @param timeout Max time to wait before giving up.
     * @return Index of the selected case, or std::nullopt if timeout or cancellation occurred.
     */
    std::optional<size_t> run_blocking(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

    /**
     * @brief Cancels a blocking wait.
     */
    void cancel();

    /**
     * @brief Checks if cancellation has been triggered.
     * @return true if cancelled.
     */
    bool is_cancelled() const;

    /**
     * @brief Gets the index of the selected case after a successful run.
     * @return Index of the chosen case, or -1 if none.
     */
    size_t selected_index() const;

    /**
     * @brief Retrieves the value received in a selected receive case.
     * @return Optional containing the received value, or nullopt if not applicable.
     */
    optional<T> received_value() const;

    /**
     * @brief Checks whether a specific case (by index) was successful.
     * @param index Index of the case to check.
     * @return true if that case succeeded.
     */
    bool case_succeeded(size_t index) const;

   private:
    enum class CaseType { SEND,
                          RECV,
                          DEFAULT };
    struct Case {
        CaseType type;
        Channel<T>* chan;
        optional<T> send_value;
        optional<T> recv_value;
        bool success = false;
    };

    vector<Case> cases_;               // List of registered cases
    optional<size_t> selected_index_;  // Index of selected case
    bool has_default_ = false;         // Flag for default case

    std::atomic<bool> cancelled_{false};  // Cancellation state

    std::condition_variable cv_;  // Used for blocking wait
    std::mutex cv_mtx_;           // Protects condition_variable access
};

#include "select.tpp"