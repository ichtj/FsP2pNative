#ifndef TIMER_H
#define TIMER_H

#include <thread>
#include <atomic>
#include <functional>

class Timer {
public:
    Timer();                    // 构造函数
    ~Timer();                   // 析构函数，自动停止定时器

    void start(int intervalMs, std::function<void()> task);  // 启动定时任务
    void stop();                                           // 停止定时任务
    bool isRunning() const;                               // 判断是否正在运行

private:
    std::atomic<bool> running;
    std::thread timerThread;
};

#endif // TIMER_H
