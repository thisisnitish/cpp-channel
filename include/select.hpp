#pragma once

#include <cassert>
#include <iostream>
#include <optional>
#include <random>
#include <vector>

#include "channel.hpp"
using namespace std;

template <typename T>

class Select {
   public:
    Select& receive(Channel<T>& chan);
    Select& send(Channel<T>& chan, const T& val);
    Select& default_case();

    // Execute selector: returns true if some case or default succeeded - Non blocking probe
    bool run();

    // Blocking version: waits until a case becomes ready, default triggers immediately.
    // Returns selected index, or std::nullopt if timeout or cancelled.
    std::optional<size_t> run_blocking(std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

    // Cancel / stop waiting
    void cancel();
    bool is_cancelled() const;

    size_t selected_index() const;
    optional<T> received_value() const;
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

    vector<Case> cases_;
    optional<size_t> selected_index_;
    bool has_default_ = false;

    std::atomic<bool> cancelled_{false};

    std::condition_variable cv_;
    std::mutex cv_mtx_;
};

#include "select.tpp"