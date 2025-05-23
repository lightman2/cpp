
// coroutine_example.cpp
#include <iostream>
#include <thread>
#include <chrono>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <coroutine>

class TaskScheduler {
private:
    std::queue<std::function<void()>> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    bool running = true;
    std::thread worker;

public:
    TaskScheduler() : worker(&TaskScheduler::run, this) {}
    
    ~TaskScheduler() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            running = false;
        }
        // coroutine_example.cpp (续)

public:
    TaskScheduler() : worker(&TaskScheduler::run, this) {}
    
    ~TaskScheduler() {
        {
            std::unique_lock<std::mutex> lock(mtx);
            running = false;
        }
        
        cv.notify_all();
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    void schedule(std::function<void()> task) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            tasks.push(std::move(task));
        }
        cv.notify_one();
    }
    
private:
    void run() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [this] { return !tasks.empty() || !running; });
                
                if (!running && tasks.empty()) {
                    break;
                }
                
                task = std::move(tasks.front());
                tasks.pop();
            }
            
            task();
        }
    }
};

// 全局调度器
TaskScheduler scheduler;

// 简单的协程等待对象
class Awaitable {
public:
    bool await_ready() { return false; }
    
    void await_suspend(std::coroutine_handle<> h) {
        // 延迟指定时间后恢复协程
        std::thread([h, this]() {
            std::this_thread::sleep_for(delay);
            scheduler.schedule([h]() { h.resume(); });
        }).detach();
    }
    
    void await_resume() {}
    
    explicit Awaitable(std::chrono::milliseconds d) : delay(d) {}
    
private:
    std::chrono::milliseconds delay;
};

// 简单的任务类
struct Task {
    struct promise_type {
        Task get_return_object() { 
            return Task(std::coroutine_handle<promise_type>::from_promise(*this)); 
        }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
    
    std::coroutine_handle<promise_type> handle;
    
    Task(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~Task() { 
        if (handle) handle.destroy(); 
    }
    
    // 非拷贝
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    
    // 可移动
    Task(Task&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }
    
    Task& operator=(Task&& other) noexcept {
        if (this != &other) {
            if (handle) handle.destroy();
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }
};

// 异步任务示例
Task async_task(const std::string& name) {
    std::cout << name << ": 任务启动在线程 " << std::this_thread::get_id() << "\n";
    
    co_await Awaitable(std::chrono::milliseconds(1000));
    std::cout << name << ": 第一阶段完成，线程 " << std::this_thread::get_id() << "\n";
    
    co_await Awaitable(std::chrono::milliseconds(500));
    std::cout << name << ": 第二阶段完成，线程 " << std::this_thread::get_id() << "\n";
    
    co_await Awaitable(std::chrono::milliseconds(300));
    std::cout << name << ": 任务完成，线程 " << std::this_thread::get_id() << "\n";
}

// 嵌套协程示例
Task nested_coroutines() {
    std::cout << "主协程: 开始在线程 " << std::this_thread::get_id() << "\n";
    
    // 等待一个协程
    co_await Awaitable(std::chrono::milliseconds(200));
    std::cout << "主协程: 启动子协程A\n";
    auto taskA = async_task("协程A");
    
    co_await Awaitable(std::chrono::milliseconds(500));
    std::cout << "主协程: 启动子协程B\n";
    auto taskB = async_task("协程B");
    
    co_await Awaitable(std::chrono::milliseconds(2000));
    std::cout << "主协程: 所有子协程应该都完成了\n";
}

int main() {
    std::cout << "=== 协程与异步设计示例 (C++20) ===\n\n";
    std::cout << "主线程ID: " << std::this_thread::get_id() << "\n\n";
    
    std::cout << "1. 单一异步任务示例\n";
    {
        auto task = async_task("单一任务");
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
    
    std::cout << "\n2. 嵌套协程示例\n";
    {
        auto task = nested_coroutines();
        std::this_thread::sleep_for(std::chrono::milliseconds(4000));
    }
    
    std::cout << "\n所有协程执行完毕\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    return 0;
}