// This is for testing the channel class

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
#include <vector>

#include "../include/channel.hpp"

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

void testing_unbuffered_channel() {
    log("Testing unbuffered channel...");
    Channel<int> channel;

    // sender thread
    thread sender([&channel]() {
        log("Sender sending 100...");
        channel.send(100);
        log("Sender sent 100."); });

    // receiver thread
    thread receiver([&channel]() {
        log("Waiting to receive...");
        optional<int> value = channel.receive();

        if (!value.has_value()) {
            log("Receiver received nothing... ");
            return;
        }
        int val = value.value();
        string v = to_string(val);
        log("Receiver received " + v); });

    log("Testing unbuffered channel complete.");

    sender.join();
    receiver.join();
}

void testing_buffered_channel() {
    log("Testing buffered channel...");
    Channel<int> channel(3);  // Create a buffered channel of size 3

    // sender thread
    thread sender([&channel]() {
        log("Sender sending 1...");
        channel.send(1);
        log("Sender sent 1.");

        log("Sender sending 2...");
        channel.send(2);
        log("Sender sent 2.");

        log("Sender sending 3...");
        channel.send(3);
        log("Sender sent 3.");

        log("Sender sending 4...");
        channel.send(4); // This will block until a receiver consumes some data
        log("Sender sent 4."); });

    // receiver thread
    thread receiver([&channel]() {
        log("Waiting to receive...");
        optional<int> value = channel.receive();
        if(!value.has_value()) {
            log("Receiver received nothing....");
            return;
        }
        int val = value.value();
        string v = to_string(val);
        log("Receiver received " + v);

        value = channel.receive();
        if (!value.has_value()) {
            log("Receiver received nothing....");
            return;
        }
        val = value.value();
        v = to_string(val);
        log("Receiver received " + v);

        value = channel.receive();
        if (!value.has_value()) {
            log("Receiver received nothing....");
            return;
        }
        val = value.value();
        v = to_string(val);
        log("Receiver received " + v);

        value = channel.receive(); // This will block until sender sends more data
        if (!value.has_value()) {
            log("Receiver received nothing...");
            return;
        }
        val = value.value();
        v = to_string(val);
        log("Receiver received " + v); });

    log("Testing buffered channel complete.");
    sender.join();
    receiver.join();
}

void testing_close_functionality() {
    log("Testing close operations...");
    Channel<int> channel(2);  // Create a buffered channel of size 2

    // sender thread
    thread sender([&channel]() {
        log("Sender sending 1...");
        channel.send(1);
        log("Sender sent 1.");

        log("Sender sending 2...");
        channel.send(2);
        log("Sender sent 2.");

        log("Sender sending 3...");
        channel.send(3);
        log("Sender sent 3.");

        log("Closing channel...");
        channel.close();
        log("Channel closed."); });

    // receiver thread
    thread receiver([&channel]() {
        try {
            log("Waiting to receive...");
            optional<int> value = channel.receive();
            if (value.has_value()) {
                string v = to_string(value.value());
                log("Receiver received " + v);
            } else {
                log("Receiver received nothing, channel is closed.");
            }

            value = channel.receive();  // Will return nullopt if closed & empty
            if (value.has_value()) {
                log("Receiver received " + to_string(value.value()));
            } else {
                log("Receiver received nothing, channel is closed.");
            }
        } catch (const std::exception& e) {
            log(string("Receiver caught exception: ") + e.what());
        }
    });

    log("Testing close operations complete.");
    sender.join();
    receiver.join();
}

void testing_try_operations() {
    log("Testing try_send and try_receive operations...");
    Channel<int> channel(2);  // Create a buffered channel of size 2

    // sender thread
    thread sender([&channel]() {
        // log("Trying to send 1...");
        if (channel.try_send(1)) {
            log("Sender sent 1.");
        } else {
            log("Sender failed to send 1.");
        }
        log("Trying to send 2...");
        if (channel.try_send(2)) {
            log("Sender sent 2.");
        } else {
            log("Sender failed to send 2.");
        }
        log("Trying to send 3...");
        if (channel.try_send(3)) {
            log("Sender sent 3.");
        } else {
            log("Sender failed to send 3.");
        }
        log("Trying to send 4...");
        if (channel.try_send(4)) {
            log("Sender sent 4.");
        } else {
            log("Sender failed to send 4.");
        }
    });

    // receiver thread
    thread receiver([&channel]() {
        log("Trying to receive...");
        optional<int> value = channel.try_receive();
        if (value.has_value()) {
            log("Receiver received " + to_string(value.value()));
        } else {
            log("Receiver received nothing.");
        }

        value = channel.try_receive();
        if (value.has_value()) {
            log("Receiver received " + to_string(value.value()));
        }

        else {
            log("Receiver received nothing.");
        }

        value = channel.try_receive();
        if (value.has_value()) {
            log("Receiver received " + to_string(value.value()));
        } else {
            log("Receiver received nothing.");
        }

        value = channel.try_receive();  // Will return nullopt if closed & empty
        if (value.has_value()) {
            log("Receiver received " + to_string(value.value()));
        } else {
            log("Receiver received nothing.");
        }
    });

    log("Testing try_send and try_receive operations complete.");
    sender.join();
    receiver.join();
}

void test_async_send_receive_immediate_match() {
    log("Testing async_send and async_receive immediate match...");
    Channel<int> channel;

    auto future_receive = channel.async_receive();
    log("Waiting for async receive...");

    // Immediately send a value
    auto future_send = channel.async_send(42);
    log("Async send initiated.");

    future_send.get();                   // wait for send to complete
    auto result = future_receive.get();  // wait for receive to complete

    assert(result.has_value() && result.value() == 42);
    log("Async send and receive completed successfully with value: " + to_string(result.value()));
}

void test_async_send_blocks_until_receive() {
    log("Testing async_send blocks until receive...");
    Channel<int> channel;

    auto future_send = channel.async_send(99);
    log("Async send initiated, waiting for receiver...");
    this_thread::sleep_for(chrono::seconds(1));  // Simulate some delay
    assert(!future_send.valid() || future_send.wait_for(chrono::seconds(0)) == future_status::timeout);
    // future_send.wait();
    log("Async send is still pending, now receiving...");

    auto future_receive = channel.async_receive();
    log("Async receive initiated.");
    auto result = future_receive.get();  // This will block until the value is sent

    assert(result.has_value() && result.value() == 99);
    log("Async send and receive completed successfully with value: " + to_string(result.value()));
}

void test_async_receive_blocks_until_send() {
    log("Testing async receive blocks until send...");
    Channel<int> ch;

    auto futureRecv = ch.async_receive();

    this_thread::sleep_for(chrono::seconds(1));  // Simulate some delay
    assert(futureRecv.wait_for(chrono::seconds(0)) == future_status::timeout);
    // futureRecv.wait();
    log("Async receive is still pending, now sending...");

    auto futureSend = ch.async_send(123);

    futureSend.get();
    auto result = futureRecv.get();
    assert(result.has_value());
    assert(result.value() == 123);

    log("Async receive blocks until send completed successfully with value: " + to_string(result.value()));
}

void test_async_receive_after_close_returns_nullopt() {
    log("Testing async receive after close returns nullopt...");
    Channel<int> ch;

    ch.close();

    auto future_receive = ch.async_receive();
    auto result = future_receive.get();
    assert(result == nullopt);
    log("Async receive after close returns nullopt completed successfully");
}

void test_async_send_after_close_fails() {
    log("Testing async send after close fails...");
    Channel<int> ch;

    ch.close();

    auto future_send = ch.async_send(10);

    try {
        future_send.get();  // this should throw
        assert(false && "Expected exception from async_send after channel close");
    } catch (const runtime_error& e) {
        log(string("Caught expected exception: ") + e.what());
    }

    log("Testing async send after close fails completed successfully...");
}

void test_multi_producer_consumer_without_select() {
    log("Testing multiple producer consumer without select in single channel...");
    constexpr int num_producers = 3;
    constexpr int items_per_producer = 50;
    constexpr int num_consumers = 4;
    const int expected_total = num_producers * items_per_producer;

    Channel<int> ch(20);  // buffered channel

    vector<int> consumed;
    mutex consumed_mtx;

    // Producers
    vector<thread> producers;
    for (int p = 0; p < num_producers; ++p) {
        producers.emplace_back([p, &ch]() {
            for (int i = 0; i < items_per_producer; ++i) {
                int val = p * 1000 + i;
                ch.send(val);
            }
        });
    }

    // Consumers
    vector<thread> consumers;
    for (int c = 0; c < num_consumers; ++c) {
        consumers.emplace_back([&]() {
            while (true) {
                auto maybe = ch.receive();
                if (!maybe.has_value()) break;  // closed and drained
                int v = *maybe;
                lock_guard lock(consumed_mtx);
                consumed.push_back(v);
            }
        });
    }

    for (auto& t : producers) t.join();
    ch.close();
    for (auto& t : consumers) t.join();

    {
        lock_guard lock(consumed_mtx);
        log("Expected=" + to_string(expected_total) + " Consumed=" + to_string(consumed.size()));
        assert((int)consumed.size() == expected_total);
        set<int> uniq(consumed.begin(), consumed.end());
        assert((int)uniq.size() == expected_total);
    }

    log("Testing multiple producer consumer without select in single channel completed...");
}

int main() {
    testing_unbuffered_channel();
    cout << "----------------------------------" << endl;
    testing_buffered_channel();
    cout << "----------------------------------" << endl;
    testing_close_functionality();
    cout << "----------------------------------" << endl;
    testing_try_operations();
    cout << "----------------------------------" << endl;
    test_async_send_receive_immediate_match();
    cout << "----------------------------------" << endl;
    test_async_send_blocks_until_receive();
    cout << "----------------------------------" << endl;
    test_async_receive_blocks_until_send();
    cout << "----------------------------------" << endl;
    test_async_receive_after_close_returns_nullopt();
    cout << "----------------------------------" << endl;
    test_async_send_after_close_fails();
    cout << "----------------------------------" << endl;
    test_multi_producer_consumer_without_select();

    return 0;
}
