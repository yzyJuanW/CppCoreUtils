#include <fmt/core.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <future>
#include <thread>

#include "utils/blocking_queue.hpp"
#include "utils/current_thread.h"
#include "utils/singleton.h"
#include "utils/thread_pool.hpp"

using namespace ywh::utils;

// test thread pool
class ThreadPoolTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // 创建线程池，使用4个工作线程
    pool = std::make_unique<ThreadPool>(16);
  }

  void TearDown() override {
    // 确保线程池在测试结束后被销毁
    pool.reset();
  }

  std::unique_ptr<ThreadPool> pool;
};

// 测试基本任务执行
TEST_F(ThreadPoolTest, BasicTaskExecution) {
  std::atomic<bool> taskExecuted{false};

  // 添加一个简单任务
  pool->Enqueue([&] { taskExecuted = true; });

  // 等待任务执行
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_TRUE(taskExecuted);
}

// 测试多个任务执行
TEST_F(ThreadPoolTest, MultipleTasks) {
  const int taskCount = 100;
  std::atomic<int> counter{0};

  // 添加多个任务
  for (int i = 0; i < taskCount; ++i) {
    pool->Enqueue([&] { counter++; });
  }

  // 等待所有任务完成
  while (counter < taskCount) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  EXPECT_EQ(taskCount, counter);
}

// 测试带参数的任务
TEST_F(ThreadPoolTest, TaskWithArguments) {
  std::atomic<int> result{0};

  // 添加带参数的任务
  pool->Enqueue([](int a, int b, std::atomic<int>& res) { res = a + b; }, 5, 7, std::ref(result));

  // 等待结果
  while (result == 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  EXPECT_EQ(12, result);
}

// 测试返回值获取（通过future）
TEST_F(ThreadPoolTest, ReturnValueViaFuture) {
  // 使用packaged_task获取返回值
  std::packaged_task<int()> task([] { return 42; });

  auto future = task.get_future();

  // 将任务加入线程池
  pool->Enqueue([&task] { task(); });

  // 等待结果
  EXPECT_EQ(42, future.get());
}

// 测试并发执行能力
TEST_F(ThreadPoolTest, Concurrency) {
  const int threadCount = 4;
  std::vector<std::future<void>> futures;
  std::vector<std::chrono::steady_clock::time_point> startTimes(threadCount);
  std::vector<std::chrono::steady_clock::time_point> endTimes(threadCount);

  // 添加需要长时间执行的任务
  for (int i = 0; i < threadCount; ++i) {
    futures.push_back(std::async(std::launch::async, [this, i, &startTimes, &endTimes] {
      pool->Enqueue([i, &startTimes, &endTimes] {
        startTimes[i] = std::chrono::steady_clock::now();
        // 模拟工作负载
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        endTimes[i] = std::chrono::steady_clock::now();
      });
    }));
  }

  // 等待所有任务提交
  for (auto& f : futures) {
    f.wait();
  }

  // 等待所有任务完成
  std::this_thread::sleep_for(std::chrono::milliseconds(150));

  // 检查任务是否并发执行
  for (int i = 0; i < threadCount; ++i) {
    for (int j = i + 1; j < threadCount; ++j) {
      // 检查任务执行时间是否有重叠
      bool concurrent = (startTimes[i] < endTimes[j]) && (startTimes[j] < endTimes[i]);
      EXPECT_TRUE(concurrent) << "Tasks " << i << " and " << j << " did not execute concurrently";
    }
  }
}

// 测试析构时等待任务完成
TEST_F(ThreadPoolTest, DestructorWaitsForTasks) {
  std::atomic<bool> taskStarted{false};
  std::atomic<bool> taskCompleted{false};

  {
    ThreadPool tempPool(1);

    // 添加一个长时间运行的任务
    tempPool.Enqueue([&] {
      taskStarted = true;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      taskCompleted = true;
    });

    // 等待任务开始
    while (!taskStarted) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }  // tempPool在此处析构

  // 验证任务已完成
  EXPECT_TRUE(taskCompleted);
}

// 测试线程池大小
TEST_F(ThreadPoolTest, PoolSize) {
  const int expectedSize = 4;
  std::atomic<int> concurrentTasks{0};
  std::atomic<int> maxConcurrentTasks{0};
  std::vector<std::future<void>> futures;

  // 添加任务来检测并发数
  for (int i = 0; i < 10; ++i) {
    futures.push_back(std::async(std::launch::async, [this, &concurrentTasks, &maxConcurrentTasks] {
      pool->Enqueue([&concurrentTasks, &maxConcurrentTasks] {
        int current = ++concurrentTasks;

        // 更新最大并发数
        int oldMax = maxConcurrentTasks;
        while (oldMax < current && !maxConcurrentTasks.compare_exchange_weak(oldMax, current)) {
        }

        // 模拟工作负载
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        --concurrentTasks;
      });
    }));
  }

  // 等待所有任务提交
  for (auto& f : futures) {
    f.wait();
  }

  // 等待所有任务完成
  while (concurrentTasks > 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // 验证最大并发数等于线程池大小
  EXPECT_EQ(expectedSize, maxConcurrentTasks);
}

void ComputeIntensiveTask() {
  volatile double result = 0.0;
  // 进行大量浮点计算
  for (int i = 0; i < 10000000; ++i) {
    result += std::sin(i) * std::cos(i) / std::exp(0.01 * i);
  }
  // 防止编译器优化掉整个循环
  (void)result;
}

TEST_F(ThreadPoolTest, ComputeIntensiveTask) {
  const int task_total_count = 1000;
  std::atomic<int> task_count(0);
  std::mutex mutex;
  std::condition_variable cv;
  for (int i = 0; i < task_total_count; i++) {
    task_count++;
    pool->Enqueue([&] {
      ComputeIntensiveTask();  // 模拟你的多线程任务
      if (--task_count == 0) {
        std::lock_guard<std::mutex> lock(mutex);
        cv.notify_one();
      }
    });
  }
  fmt::print("now task count : {}\n", task_count);
  // 等待所有任务完成
  std::unique_lock<std::mutex> lock(mutex);
  cv.wait(lock, [&] { return task_count == 0; });

  fmt::print("all task finish!!!!  task count : {}\n", task_count);
}

TEST_F(ThreadPoolTest, TestThreadId) {
  const int task_total_count = 50;
  for (int i = 0; i < task_total_count; i++) {
    pool->Enqueue(
        [&, i]() { fmt::print("i = {}, thread id = {}\n", i, current_thread::GetTid()); });
  }
}

TEST_F(ThreadPoolTest, TestPoolGetFuture) {
  const int task_total_count = 50;

  std::vector<std::future<int>> futures;

  for (int i = 0; i < task_total_count; i++) {
    if (i % 2 == 0) {
      futures.push_back(pool->EnqueueWithFuture([i] {
        fmt::print("run the enqueue with future, i = {}\n", i);
        return i * i;
      }));
    } else {
      pool->Enqueue([i] {
        fmt::print("run the enqueue no future, i = {}\n", i);
        return i;
      });
    }
  }
  for (auto& future : futures) {
    fmt::print("get the res : {}\n", future.get());
  }
}

TEST_F(ThreadPoolTest, TestPoolGetFutureWait) {
  const int task_total_count = 50;

  std::atomic<int> task_count(0);
  std::vector<std::future<void>> futures;

  for (int i = 0; i < task_total_count; i++) {
    futures.push_back(pool->EnqueueWithFuture([&](int i) {
      fmt::print("run the enqueue with future, i = {}, count = {}\n", i, task_count++);
    }, i));
  }
  for (auto& future : futures) {
    future.wait();
  }
  fmt::print("task count = {}, tast total count = {}\n", task_count, task_total_count);
}