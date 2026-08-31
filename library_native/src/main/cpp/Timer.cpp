#include "Timer.h"

#include <utility>

Timer::Timer() {
    worker = std::thread([this]() { run(); });
}

Timer::~Timer() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        running = false;
        shuttingDown = true;
        ++generation;
    }
    condition.notify_all();
    if (worker.joinable()) worker.join();
}

void Timer::start(int intervalMs, std::function<void()> task) {
    if (!task) return;

    {
        std::unique_lock<std::mutex> lock(mutex);
        if (std::this_thread::get_id() != executingThread) {
            condition.wait(lock, [this]() { return !executing; });
        }
        currentTask = std::move(task);
        interval = std::chrono::milliseconds(intervalMs > 0 ? intervalMs : 1);
        running = true;
        ++generation;
    }
    condition.notify_all();
}

void Timer::stop() {
    std::unique_lock<std::mutex> lock(mutex);
    running = false;
    ++generation;
    condition.notify_all();

    if (std::this_thread::get_id() != executingThread) {
        condition.wait(lock, [this]() { return !executing; });
    }
}

bool Timer::isRunning() const {
    std::lock_guard<std::mutex> lock(mutex);
    return running;
}

void Timer::run() {
    std::unique_lock<std::mutex> lock(mutex);
    while (!shuttingDown) {
        condition.wait(lock, [this]() { return shuttingDown || running; });
        if (shuttingDown) return;

        const unsigned long long activeGeneration = generation;
        auto task = currentTask;
        executing = true;
        executingThread = std::this_thread::get_id();

        lock.unlock();
        bool taskFailed = false;
        try {
            task();
        } catch (...) {
            taskFailed = true;
        }
        lock.lock();

        executing = false;
        executingThread = std::thread::id();
        if (taskFailed && generation == activeGeneration) {
            running = false;
            ++generation;
        }
        condition.notify_all();

        if (shuttingDown) return;
        if (!running || generation != activeGeneration) continue;

        condition.wait_for(lock, interval, [this, activeGeneration]() {
            return shuttingDown || !running || generation != activeGeneration;
        });
    }
}
