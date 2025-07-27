// This is for testing the channel class

#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>

#include "../include/channel.hpp"

using namespace std;

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

int main()
{
    testing_unbuffered_channel();
    return 0;
}
