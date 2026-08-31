#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

class Timer {
public:
    Timer();
    ~Timer();

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    void start(int intervalMs, std::function<void()> task);
    void stop();
    bool isRunning() const;

private:
    void run();

    mutable std::mutex mutex;
    std::condition_variable condition;
    std::thread worker;
    std::thread::id executingThread;
    std::function<void()> currentTask;
    std::chrono::milliseconds interval{1};
    unsigned long long generation = 0;
    bool running = false;
    bool executing = false;
    bool shuttingDown = false;
};

#endif // TIMER_H
