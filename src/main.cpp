// This is a very basic usecase example for channel along with select
// For more information look at the docs and test cases

#include <cassert>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

#include "../include/channel.hpp"
#include "../include/select.hpp"

using namespace std;

int main() {
    Channel<int> ch1(1), ch2(1);
    ch1.send(10);
    Select<int> sel;
    sel.receive(ch1).receive(ch2).default_case();

    if (sel.run()) {
        size_t idx = sel.selected_index();
        if (idx < 2) {
            int val = *sel.received_value();
            cout << "Received: " << val << " from case " << idx << "\n";
        } else {
            cout << "Default taken\n";
        }
    }

    return 0;
}
