#pragma once

template <typename T>
Select<T>& Select<T>::receive(Channel<T>& chan) {
    cases_.push_back(Case{CaseType::RECV, &chan, nullopt, nullopt, false});
    return *this;
}

template <typename T>
Select<T>& Select<T>::send(Channel<T>& chan, const T& val) {
    cases_.push_back(Case{CaseType::SEND, &chan, val, nullopt, false});
    return *this;
}

template <typename T>
Select<T>& Select<T>::default_case() {
    has_default_ = true;
    return *this;
}

template <typename T>
bool Select<T>::run() {
    vector<size_t> ready_indices;

    for (size_t i = 0; i < cases_.size(); i++) {
        Case& c = cases_[i];
        if (c.type == CaseType::RECV) {
            auto val = c.chan->try_receive();
            if (val.has_value()) {
                c.recv_value = val.value();
                c.success = true;
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
        random_device rd;
        mt19937 gen(rd());  // Mersenne Twister engine (a high-quality pseudo-random generator)
        // Random selection on ready cases - Fairness on multiple ready cases
        uniform_int_distribution<size_t> dist(0, ready_indices.size() - 1);
        selected_index_ = ready_indices[dist(gen)];
        return true;
    }

    if (has_default_) {
        selected_index_ = cases_.size();  // default case index
        return true;
    }

    return false;
}

template <typename T>
size_t Select<T>::selected_index() const {
    return selected_index_.value_or(static_cast<size_t>(-1));
}

template <typename T>
optional<T> Select<T>::received_value() const {
    if (!selected_index_ || *selected_index_ >= cases_.size()) return nullopt;
    size_t idx = *selected_index_;
    return cases_[idx].recv_value;
}

template <typename T>
bool Select<T>::case_succeeded(size_t index) const {
    if (index >= cases_.size()) return false;
    return cases_[index].success;
}
