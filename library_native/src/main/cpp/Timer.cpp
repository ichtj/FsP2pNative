#include "Timer.h"
#include <chrono>

Timer::Timer() : running(false) {}

Timer::~Timer() {
    stop(); // 析构时确保线程停止
}

void Timer::start(int intervalMs, std::function<void()> task) {
    if (running) return;
    running = true;

    timerThread = std::thread([=]() {
        while (running) {
            task(); // ✅ 先执行
            std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
        }
    });

    timerThread.detach();
}


void Timer::stop() {
    running = false;
}

bool Timer::isRunning() const {
    return running;
}
