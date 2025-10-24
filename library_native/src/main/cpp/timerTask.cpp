#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>

// 定时器控制变量
std::atomic<bool> g_timerRunning(false);
std::thread g_timerThread;

// 定时任务函数
void timerTask(int intervalMs) {
    while (g_timerRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));

        if (!g_timerRunning.load()) break; // 停止标志检查
        std::cout << "定时任务执行中..." << std::endl; // 你要执行的逻辑
    }
}

// 启动定时器
void startTimer(int intervalMs) {
    if (g_timerRunning.load()) {
        std::cout << "定时器已在运行中" << std::endl;
        return;
    }

    g_timerRunning.store(true);
    g_timerThread = std::thread(timerTask, intervalMs);
    g_timerThread.detach(); // 分离线程，使其后台运行
    std::cout << "定时器已启动" << std::endl;
}

// 停止定时器
void stopTimer() {
    if (!g_timerRunning.load()) {
        std::cout << "定时器未运行" << std::endl;
        return;
    }

    g_timerRunning.store(false);
    std::cout << "定时器已停止" << std::endl;
}

int main() {
    startTimer(1000); // 每隔 1000ms 执行一次
    std::this_thread::sleep_for(std::chrono::seconds(5));
    stopTimer();
    std::this_thread::sleep_for(std::chrono::seconds(2)); // 观察是否停止
    return 0;
}
