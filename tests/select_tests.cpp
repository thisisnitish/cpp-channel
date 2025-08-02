// This is for testing the selectors

#include <cassert>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <unordered_set>

#include "../include/channel.hpp"
#include "../include/select.hpp"

using namespace std;

// TODO: Refactor the test cases in a much better way...
void log(const std::string& message) {
    // Get current time
    auto now = std::chrono::system_clock::now();

    // Extract time_t and milliseconds
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms_part = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now.time_since_epoch()) %
                   1000;

    // Format to tm
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time_t_now);
#else
    localtime_r(&time_t_now, &tm);
#endif

    // Get hashed thread ID
    size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());

    // Compose output
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms_part.count()
        << " [Thread " << thread_id << "] "
        << message;

    std::cout << oss.str() << std::endl;
}

void test_select_recv_ready() {
    log("Testing select receive ready....");
    Channel<int> ch1(1), ch2(1);
    ch1.send(10);  // ch1 has the data

    Select<int> sel;
    sel.receive(ch1).receive(ch2);

    bool ok = sel.run();
    assert(ok);

    size_t idx = sel.selected_index();
    assert(idx == 0);
    auto val = sel.received_value();
    assert(val.has_value() && val.value() == 10);

    log("Testing select receive ready completed....");
}

void test_select_send_ready() {
    log("Testing select send ready....");
    Channel<int> ch1(1), ch2(1);
    // both channels have space for send (buffered capacity 1)
    Select<int> sel;
    sel.send(ch1, 5).send(ch2, 6);

    bool ok = sel.run();
    assert(ok);

    size_t idx = sel.selected_index();
    assert((idx == 0 || idx == 1));
    assert(sel.case_succeeded(idx));

    log("Testing select send ready completed....");
}

void test_select_default_case() {
    log("Testing select default case....");
    Channel<int> ch1(1);
    // no data: receive wouldn't be ready
    Select<int> sel;
    sel.receive(ch1).default_case();

    bool ok = sel.run();
    assert(ok);
    assert(sel.selected_index() == 1);  // default case index

    log("Testing select default case completed....");
}

void test_select_multiple_ready_randomness() {
    log("Testing select multiple ready randomness...");
    Channel<int> ch1(1), ch2(1);
    ch1.send(1);
    ch2.send(2);

    Select<int> sel;
    sel.receive(ch1).receive(ch2);

    bool ok = sel.run();
    assert(ok);
    size_t idx = sel.selected_index();
    assert((idx == 0 || idx == 1));
    assert(sel.case_succeeded(idx));

    auto val = sel.received_value();
    assert(val.has_value());
    int v = val.value();
    assert(v == 1 || v == 2);

    log("Testing select multiple ready randomness completed...");
}

void test_select_with_async_send() {
    log("Testing select with asynchronous send...");
    Channel<int> ch1(1), ch2(1);
    // ch2 has no data; ch1 will get async send
    Select<int> sel;

    // Start async send to ch1
    auto send_future = ch1.async_send(99);  // will complete immediately (buffered)
    sel.receive(ch2).receive(ch1);          // ch1 should have data after async_send

    // Wait a bit to ensure async_send has pushed value
    send_future.get();  // ensure it's done

    bool ok = sel.run();
    assert(ok);
    size_t idx = sel.selected_index();
    // check if it has picked receive(ch1), which is index 1
    assert(idx == 1);
    auto val = sel.received_value();
    assert(val.has_value() && val.value() == 99);

    log("Testing select with asynchronous send completed...");
}

void test_select_async_receive_with_default() {
    log("Testing select with asynchronous receive with default...");
    Channel<int> ch(1);
    // No data; async_receive will block in background
    Select<int> sel;
    sel.receive(ch).default_case();

    bool ok = sel.run();
    assert(ok);
    // Since it receives no data, it takes the default case (index == cases_.size())
    assert(sel.selected_index() == 1);

    // Now send asynchronously and rerun select to receive
    auto recv_future = ch.async_receive();
    this_thread::sleep_for(chrono::seconds(2));  // let async stall

    // send the value
    ch.send(123);

    // Wait to ensure value delivered
    auto val = recv_future.get();
    assert(val.has_value() && val.value() == 123);

    log("Testing select with asynchronous receive with default completed...");
}

void test_select_recv_after_close() {
    log("Testing select receive after closed...");
    Channel<int> ch1(1), ch2(1);
    ch1.send(5);
    ch1.close();  // ch1 has one value then closed

    Select<int> sel;
    sel.receive(ch1).receive(ch2).default_case();

    bool ok = sel.run();
    assert(ok);
    size_t idx = sel.selected_index();
    // Should receive from ch1 first
    assert(idx == 0);
    auto val = sel.received_value();
    assert(val.has_value() && val.value() == 5);

    // Next select should see default (because ch1 drained, ch2 empty)
    // It will move to default case, cuz ch1 is closed and ch2 has no value whatsoever
    Select<int> sel2;
    sel2.receive(ch1).receive(ch2).default_case();
    bool ok2 = sel2.run();
    assert(ok2);
    assert(sel2.selected_index() == 2);  // default

    log("Testing select receive after closed completed...");
}

void test_select_multiple_async_receives() {
    log("Testing select multiple asynchronous receives...");
    Channel<int> ch1(1), ch2(1);

    Select<int> sel;
    sel.receive(ch1).receive(ch2);

    // Fire async sends to both; they race
    auto f1 = async(launch::async, [&]() { ch1.send(10); });
    auto f2 = async(launch::async, [&]() { ch2.send(20); });

    // Wait a bit to ensure one of them has delivered
    this_thread::sleep_for(chrono::milliseconds(10));

    bool ok = sel.run();
    assert(ok);
    size_t idx = sel.selected_index();
    assert(idx == 0 || idx == 1);
    auto val = sel.received_value();

    assert(val.has_value());
    int v = val.value();
    assert(v == 10 || v == 20);

    f1.get();
    f2.get();

    log("Testing select multiple asynchronous receives completed...");
}

void test_fan_in_with_select_blocking_cv() {
    std::cout << "[Test] fan-in with select (blocking cv)\n";
    constexpr int per = 10;
    Channel<int> ch1(10), ch2(10);

    std::atomic<bool> p1_done{false}, p2_done{false};
    std::thread p1([&]() {
        for (int i = 0; i < per; ++i) ch1.send(100 + i);
        ch1.close();
        p1_done.store(true);
        std::cout << "[Producer 1] done\n";
    });
    std::thread p2([&]() {
        for (int i = 0; i < per; ++i) ch2.send(200 + i);
        ch2.close();
        p2_done.store(true);
        std::cout << "[Producer 2] done\n";
    });

    std::set<int> collected;
    std::mutex collected_mtx;
    const int expected_total = 2 * per;
    auto overall_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);

    while ((int)collected.size() < expected_total) {
        if (std::chrono::steady_clock::now() >= overall_deadline) {
            std::cerr << "[Test] overall timeout\n";
            break;
        }

        Select<int> sel;
        sel.receive(ch1).receive(ch2);  // no default

        auto opt_idx = sel.run_blocking(std::chrono::milliseconds(1000));
        if (!opt_idx.has_value()) continue;

        size_t idx = *opt_idx;
        auto val = sel.received_value();
        if ((idx == 0 || idx == 1) && val.has_value()) {
            int v = *val;
            std::lock_guard lock(collected_mtx);
            if (collected.insert(v).second) {
                std::cout << "[Select] picked case " << idx << " value=" << v << "\n";
            }
        }
    }

    // Drain any remaining values from channels after loop ends
    while (auto v = ch1.try_receive()) {
        std::lock_guard lock(collected_mtx);
        collected.insert(*v);
        std::cerr << "[Drain] ch1 leftover: " << *v << "\n";
    }
    while (auto v = ch2.try_receive()) {
        std::lock_guard lock(collected_mtx);
        collected.insert(*v);
        std::cerr << "[Drain] ch2 leftover: " << *v << "\n";
    }

    p1.join();
    p2.join();

    // std::cerr << "[Diagnostic] draining ch1 directly:\n";
    // while (auto v = ch1.try_receive()) std::cerr << "  ch1 leftover: " << *v << "\n";
    // std::cerr << "[Diagnostic] draining ch2 directly:\n";
    // while (auto v = ch2.try_receive()) std::cerr << "  ch2 leftover: " << *v << "\n";

    {
        std::lock_guard lock(collected_mtx);
        std::cout << "[Test] Expected=" << expected_total
                  << " Collected=" << collected.size() << "\n";
        assert((int)collected.size() == expected_total);
    }

    std::cout << "✅ test_fan_in_with_select_blocking_cv passed\n";
}

int main() {
    test_select_recv_ready();
    cout << "----------------------------------" << endl;
    test_select_send_ready();
    cout << "----------------------------------" << endl;
    test_select_default_case();
    cout << "----------------------------------" << endl;
    test_select_multiple_ready_randomness();
    cout << "----------------------------------" << endl;
    test_select_with_async_send();
    cout << "----------------------------------" << endl;
    test_select_async_receive_with_default();
    cout << "----------------------------------" << endl;
    test_select_recv_after_close();
    cout << "----------------------------------" << endl;
    test_select_multiple_async_receives();
    cout << "----------------------------------" << endl;
    test_fan_in_with_select_blocking_cv();

    return 0;
}
