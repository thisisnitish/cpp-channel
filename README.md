# CPP Channel

CPP channel is an attempt to provide an API for Go like channels in CPP. It supports buffered and unbuffered 
communication, blocking and non-blocking operations, async patterns, and a select mechanism.

## Core Features
### Channels
- Buffered and unbuffered
- Blocking send/receive
- Non-blocking `try_send` / `try_receive`
- Async send/receive (`std::future`)
- Multiple producers / consumers
- Close semantics

### Select
- Wait on multiple channel operations
- Optional default case
- Blocking and non-blocking modes
- Cancellation support

## Definition and Behaviour Guarantees

### Channel
- A typed pipe like structure for passing messages between threads.
- **Unbuffered channel:** send blocks until a receiver is ready and vice versa.
- **Buffered channel:** send blocks only if the buffer is full; receive blocks only if the buffer is empty.
- Closing a channel disallows further sends but allows receiving remaining buffered data.
- Thread-safe for multiple senders and receivers.

### Select
- Waits on multiple channel operations, proceeding with exactly **one** ready case.
- If multiple cases are ready, one is chosen randomly (no fairness guarantee).
- Default case executes immediately if no case is ready.
- Blocking mode waits until any case is ready or timeout/cancellation occurs.

## Installation / Usage
- Copy `channel.hpp`, `channel.tpp`, `select.hpp`, and `select.tpp` from the `include` directory into your
project and use them
- Run `make` to build local examples and tests.
- To run tests:
    ```bash
    build/example
    build/channel_test
    build/select_test
    ```


## Examples
### 1. Basic Channel: Send and Receive(Blocking)
```cpp
#include "../include/channel.hpp"
#include <iostream>
#include <thread>
using namespace std;

int main() {
    Channel<int> ch; // unbuffered

    // Sender thread
    thread sender([&]() {
        ch.send(42);
        ch.close();
    });

    // Receiver (main thread)
    auto val = ch.receive();
    if (val) {
        cout << "Received: " << *val << "\n"; // prints 42
    }

    sender.join();
    return 0;
}
```

### 2. Buffered Channel
```cpp
Channel<string> ch(2); // buffer size 2
ch.send("first");
ch.send("second");
// buffer is full now; next send would block until a receive consumes one

auto a = ch.receive();
auto b = ch.receive();
cout << *a << ", " << *b << "\n";
```

### 3. Non-blocking Channel
```cpp
Channel<int> ch(1);
if (ch.try_send(100)) {
    cout << "Sent 100 without blocking\n";
}
if (auto v = ch.try_receive()) {
    cout << "Try received: " << *v << "\n";
}
```

### 4. Async Send / Receive
```cpp
Channel<string> ch(0); // unbuffered

auto fut_send = ch.async_send("hello");
auto fut_recv = ch.async_receive();

fut_send.wait();
auto received = fut_recv.get();
if (received) {
    cout << "Async received: " << *received << "\n";
}
```

### 5. Select with `run()` (non-blocking)
```cpp
Channel<int> ch1(1), ch2(1);
ch1.send(10); // only ch1 has a value

Select<int> sel;
sel.receive(ch1).receive(ch2);

if (sel.run()) {
    auto idx = sel.selected_index();
    auto val = sel.received_value();
    cout << "Select picked channel " << idx << " with value " << *val << "\n";
} else {
    cout << "No channel ready\n";
}
```

### 6. Select with `run_blocking()` (blocking until one is ready)
```cpp
Channel<int> ch1(1), ch2(1);
ch2.send(77); // only ch2 has a value

Select<int> sel;
sel.receive(ch1).receive(ch2);

auto opt_idx = sel.run_blocking(chrono::milliseconds(500));
if (opt_idx) {
    cout << "Blocking select got value " << *sel.received_value()
              << " from channel " << *opt_idx << "\n";
} else {
    cout << "Timeout or cancelled\n";
}
```
### 7. Select with Default Case
```cpp
Channel<int> ch; // empty

Select<int> sel;
sel.receive(ch).default_case();

if (sel.run()) {
    size_t idx = sel.selected_index();
    if (idx == 0) {
        cout << "Received from channel: " << *sel.received_value() << "\n";
    } else {
        cout << "Default branch taken (no channel ready)\n";
    }
}
```

### 8. Fan-In: Multiple Producers into One Consumer via Select
```cpp
Channel<int> ch1(2), ch2(2);
set<int> collected;

thread p1([&]() {
    for (int i = 0; i < 5; ++i) ch1.send(100 + i);
    ch1.close();
});
thread p2([&]() {
    for (int i = 0; i < 5; ++i) ch2.send(200 + i);
    ch2.close();
});

while (collected.size() < 10) {
    Select<int> sel;
    sel.receive(ch1).receive(ch2);
    auto opt = sel.run_blocking(chrono::milliseconds(1000));
    if (!opt) continue;
    if (auto v = sel.received_value()) {
        collected.insert(*v);
        cout << "Collected: " << *v << "\n";
    }
}

// Drain leftovers if any (to guarantee completeness)
while (auto v = ch1.try_receive()) collected.insert(*v);
while (auto v = ch2.try_receive()) collected.insert(*v);

p1.join();
p2.join();
cout << "Total collected: " << collected.size() << "\n";
```