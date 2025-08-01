#pragma once

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

    // Execute selector: returns true if some case or default succeeded
    bool run();

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
};

#include "select.tpp"