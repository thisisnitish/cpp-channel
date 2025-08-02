#pragma once

// Register a receive case
template <typename T>
Select<T>& Select<T>::receive(Channel<T>& chan) {
    cases_.push_back(Case{CaseType::RECV, &chan, std::nullopt, std::nullopt, false});
    return *this;
}

// Register a send case
template <typename T>
Select<T>& Select<T>::send(Channel<T>& chan, const T& val) {
    cases_.push_back(Case{CaseType::SEND, &chan, val, std::nullopt, false});
    return *this;
}

// Register a default case
template <typename T>
Select<T>& Select<T>::default_case() {
    has_default_ = true;
    return *this;
}

// Try to run any ready case (non-blocking)
template <typename T>
bool Select<T>::run() {
    if (is_cancelled()) return false;

    // Clear previous selection state
    selected_index_.reset();
    for (auto& c : cases_) {
        c.success = false;
        c.recv_value.reset();
    }

    std::vector<std::size_t> ready_indices;

    for (std::size_t i = 0; i < cases_.size(); i++) {
        Case& c = cases_[i];
        if (c.type == CaseType::RECV) {
            // auto val = c.chan->try_receive();
            if (c.chan->is_receive_ready()) {
                // c.recv_value = val.value();
                // c.success = true;
                ready_indices.push_back(i);
            }
        } else if (c.type == CaseType::SEND) {
            if (c.chan->try_send(*c.send_value)) {
                c.success = true;
                ready_indices.push_back(i);
            }
        }
    }

    if (!ready_indices.empty()) {
        std::random_device rd;
        std::mt19937 gen(rd());  // Mersenne Twister engine (a high-quality pseudo-random generator)
        // Random selection on ready cases - Fairness on multiple ready cases
        std::uniform_int_distribution<std::size_t> dist(0, ready_indices.size() - 1);
        selected_index_ = ready_indices[dist(gen)];

        Case& chosen = cases_[*selected_index_];
        auto val = chosen.chan->try_receive();  // Should succeed due to earlier readiness check
        if (val.has_value()) {
            chosen.recv_value = *val;
            chosen.success = true;
            return true;
        }
        // Edge case: race condition where data is no longer available
        selected_index_.reset();
        return false;
    }

    if (has_default_) {
        selected_index_ = cases_.size();  // Use a cases_.size() for default case
        return true;
    }

    return false;
}

// Blocking run with optional timeout
template <typename T>
std::optional<std::size_t> Select<T>::run_blocking(std::chrono::milliseconds timeout) {
    auto deadline = std::chrono::steady_clock::now() + timeout;

    // Register for wakeup notifications
    for (auto& c : cases_) {
        if (c.chan) {
            c.chan->add_notifier(&cv_);
        }
    }

    while (true) {
        if (is_cancelled()) return std::nullopt;

        if (run()) {
            return selected_index();
        }

        if (std::chrono::steady_clock::now() >= deadline) {
            return std::nullopt;
        }

        std::unique_lock lock(cv_mtx_);
        // Wait for notification or timeout
        cv_.wait_until(lock, deadline, [this]() {
            return is_cancelled();
        });
        // After wakeup, loop to reevaluate case readiness
    }
}

// Trigger cancellation
template <typename T>
void Select<T>::cancel() {
    cancelled_.store(true, std::memory_order_relaxed);
}

// Check if the select operation was cancelled
template <typename T>
bool Select<T>::is_cancelled() const {
    return cancelled_.load(std::memory_order_relaxed);
}

// Get the index of selected case
template <typename T>
size_t Select<T>::selected_index() const {
    return selected_index_.value_or(static_cast<size_t>(-1));
}

// Retrieve received value if applicable
template <typename T>
std::optional<T> Select<T>::received_value() const {
    if (!selected_index_ || !selected_index_.has_value() || *selected_index_ >= cases_.size())
        return std::nullopt;
    size_t idx = *selected_index_;
    return cases_[idx].recv_value;
}

// Check if a specific case was successful or not
template <typename T>
bool Select<T>::case_succeeded(std::size_t index) const {
    if (index >= cases_.size()) return false;
    return cases_[index].success;
}
