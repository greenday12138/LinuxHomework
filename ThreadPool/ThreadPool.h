#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

class ThreadPool {
public:
  // 构造函数，初始化线程池，创建指定数量的工作线程
  explicit ThreadPool(size_t numThreads) : stop(false), runningTasks(0) {
    // 创建指定数量的工作线程
    for (size_t i = 0; i < numThreads; ++i) {
      workers.emplace_back([this]() {
        while (true) {
          Task task;
          {
            std::unique_lock<std::mutex> lock(queueMutex);
            // 等待直到有任务或线程池停止
            condition.wait(lock,
                           [this]() { return stop.load() || !tasks.empty(); });

            // 如果线程池停止且任务队列为空，则退出线程
            if (stop.load() && tasks.empty())
              return;

            // 从任务队列中取出一个任务
            task = std::move(tasks.front());
            tasks.pop();
          }

          // 增加当前正在运行的任务计数
          runningTasks.fetch_add(1, std::memory_order_relaxed);
          task(); // 执行任务
          // 任务完成后减少正在运行的任务计数
          runningTasks.fetch_sub(1, std::memory_order_relaxed);
        }
      });
    }
  }

  // 析构函数，停止线程池并等待所有线程完成工作
  ~ThreadPool() {
    stop.store(true);       // 标记线程池停止
    condition.notify_all(); // 通知所有线程退出
    for (std::thread &worker : workers) {
      worker.join(); // 等待所有线程完成
    }
  }

  // 检查线程池是否完成所有任务
  bool finish() {
    std::lock_guard<std::mutex> lock(queueMutex);
    // 判断任务队列是否为空且当前没有正在运行的任务
    return runningTasks.load(std::memory_order_relaxed) == 0 && tasks.empty();
  }

  // 向线程池添加一个新的任务，返回一个future，用于获取任务的执行结果
  template <typename F, typename... Args>
  auto enqueue(F &&f, Args &&...args) -> std::future<decltype(std::forward<F>(
                                          f)(std::forward<Args>(args)...))> {
    using ReturnType = decltype(std::forward<F>(f)(
        std::forward<Args>(args)...)); // 任务返回值类型

    // 创建一个task包装任务
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    // 获取任务的future，任务完成时可以获取返回值
    std::future<ReturnType> result = task->get_future();
    {
      std::unique_lock<std::mutex> lock(queueMutex); // 锁住队列
      if (stop.load()) { // 如果线程池已经停止，抛出异常
        throw std::runtime_error(
            "ThreadPool is stopped, cannot enqueue tasks.");
      }
      // 将任务添加到队列中
      tasks.emplace([task]() { (*task)(); });
    }
    condition.notify_one(); // 唤醒一个等待中的线程来执行任务
    return result;          // 返回future，用于获取任务结果
  }

private:
  using Task = std::function<void()>; // 定义Task类型为无返回值的可调用对象

  std::vector<std::thread> workers;  // 存储线程池中的工作线程
  std::queue<Task> tasks;            // 存储待执行的任务队列
  std::mutex queueMutex;             // 用于保护任务队列的互斥锁
  std::condition_variable condition; // 条件变量，用于线程间同步
  std::atomic<bool> stop; // 原子标志，表示线程池是否停止
  std::atomic<size_t> runningTasks; // 原子计数器，表示当前正在运行的任务数量
};