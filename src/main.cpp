// This is for testing the channel class

#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>

#include "../include/channel.hpp"

using namespace std;

// TODO: May be try to lock the logs for better understanding of the output
void log(const std::string &message)
{
    // Get current time with milliseconds
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) %
              1000;

    std::time_t t_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t_c);
#else
    localtime_r(&t_c, &tm);
#endif

    // Hash thread id to get a numeric ID
    auto thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());

    // Print timestamp, thread id, and message
    std::cout << std::put_time(&tm, "%H:%M:%S")
              << '.' << std::setfill('0') << std::setw(3) << ms.count()
              << " [Thread " << thread_id << "] "
              << message << std::endl;
}

void testing_unbuffered_channel()
{
    log("Testing unbuffered channel...");
    Channel<int> channel;

    // sender thread
    thread sender([&channel]()
                  {
        log("Sender sending 100...");
        channel.send(100);
        log("Sender sent 100."); });

    // receiver thread
    thread receiver([&channel]()
                    {
        log("Waiting to receive...");
        int value = channel.receive();
        string v = to_string(value);
        log("Receiver received " + v); });

    log("Testing unbuffered channel complete.");

    sender.join();
    receiver.join();
}

void testing_buffered_channel()
{
    log("Testing buffered channel...");
    Channel<int> channel(3); // Create a buffered channel of size 3

    // sender thread
    thread sender([&channel]()
                  {
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
    thread receiver([&channel]()
                    {
        log("Waiting to receive...");
        int value = channel.receive();
        string v = to_string(value);
        log("Receiver received " + v);

        value = channel.receive();
        v = to_string(value);
        log("Receiver received " + v);

        value = channel.receive();
        v = to_string(value);
        log("Receiver received " + v);

        value = channel.receive(); // This will block until sender sends more data
        v = to_string(value);
        log("Receiver received " + v); });

    log("Testing buffered channel complete.");
    sender.join();
    receiver.join();
}

int main()
{
    testing_unbuffered_channel();
    cout << "----------------------------------" << endl;
    testing_buffered_channel();
    return 0;
}
