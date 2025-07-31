// This is for testing the channel class

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

#include "../include/channel.hpp"

using namespace std;

// TODO: May be try to lock the logs for better understanding of the output
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

int main() {
    testing_unbuffered_channel();
    cout << "----------------------------------" << endl;
    testing_buffered_channel();
    cout << "----------------------------------" << endl;
    testing_close_functionality();
    cout << "----------------------------------" << endl;
    testing_try_operations();

    return 0;
}
