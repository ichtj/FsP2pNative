#include "Timer.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

namespace {

using namespace std::chrono_literals;

bool stopIsBarrier() {
    Timer timer;
    std::atomic<int> calls{0};
    timer.start(2, [&calls]() { ++calls; });
    std::this_thread::sleep_for(30ms);
    timer.stop();

    const int stoppedAt = calls.load();
    std::this_thread::sleep_for(30ms);
    return stoppedAt > 0 && calls.load() == stoppedAt && !timer.isRunning();
}

bool repeatedStartStopDoesNotLeakWorkers() {
    Timer timer;
    std::atomic<int> calls{0};
    std::vector<std::thread> controllers;
    for (int thread = 0; thread < 4; ++thread) {
        controllers.emplace_back([&timer, &calls]() {
            for (int iteration = 0; iteration < 20; ++iteration) {
                timer.start(1, [&calls]() { ++calls; });
                std::this_thread::sleep_for(2ms);
                timer.stop();
            }
        });
    }
    for (auto& controller : controllers) controller.join();
    timer.stop();

    const int stoppedAt = calls.load();
    std::this_thread::sleep_for(20ms);
    return calls.load() == stoppedAt && !timer.isRunning();
}

bool taskCanStopItself() {
    Timer timer;
    std::atomic<int> calls{0};
    timer.start(1, [&timer, &calls]() {
        ++calls;
        timer.stop();
    });
    std::this_thread::sleep_for(30ms);
    timer.stop();
    return calls.load() == 1 && !timer.isRunning();
}

} // namespace

int main() {
    if (!stopIsBarrier()) {
        std::cerr << "stopIsBarrier failed\n";
        return 1;
    }
    if (!repeatedStartStopDoesNotLeakWorkers()) {
        std::cerr << "repeatedStartStopDoesNotLeakWorkers failed\n";
        return 2;
    }
    if (!taskCanStopItself()) {
        std::cerr << "taskCanStopItself failed\n";
        return 3;
    }

    std::cout << "Timer tests passed\n";
    return 0;
}
