#include <fmt/core.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>

#include "utils/blocking_queue.hpp"
#include "utils/singleton.h"
#include "utils/thread_pool.hpp"


using namespace ywh::utils;

// test thread pool
class ThreadPoolTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // 创建线程池，使用4个工作线程
    pool = std::make_unique<ThreadPool>(4);
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