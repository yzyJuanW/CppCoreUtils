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
TEST(MyTestSuite, MyTestCase) { EXPECT_EQ(2, 1 + 1); }

class TestA : public ywh::utils::Singleton<TestA> {
  friend class ywh::utils::Singleton<TestA>;

 public:
  std::string GetName() { return m_name; }

 private:
  TestA() : m_name("666") {}
  ~TestA() { fmt::print("~TestA : {}", m_name); }

 private:
  std::string m_name;
};

TEST(Singleton, Singleton_TestSin_Test) {
  TestA* a = TestA::Instance();
  EXPECT_EQ(a->GetName(), "666");
}


// 测试夹具
class BlockingQueueTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // 测试前的设置代码
  }

  void TearDown() override {
    // 测试后的清理代码
  }

  BlockingQueue<int> int_queue;
  BlockingQueue<std::string> string_queue;
};

// 测试基本Push和Pop功能
TEST_F(BlockingQueueTest, BasicPushPop) {
  int value = 42;
  int result = 0;

  // 测试Push和Pop
  int_queue.Push(value);
  EXPECT_TRUE(int_queue.Pop(result));
  EXPECT_EQ(value, result);
}

// 测试字符串类型
TEST_F(BlockingQueueTest, StringType) {
  std::string testStr = "Hello, World!";
  std::string resultStr;

  string_queue.Push(testStr);
  EXPECT_TRUE(string_queue.Pop(resultStr));
  EXPECT_EQ(testStr, resultStr);
}

// 测试多个元素顺序
TEST_F(BlockingQueueTest, MultipleElementsOrder) {
  const int testValues[] = {1, 2, 3, 4, 5};
  int result = 0;

  // 按顺序添加元素
  for (int value : testValues) {
    int_queue.Push(value);
  }

  // 检查是否按顺序取出
  for (int expected : testValues) {
    EXPECT_TRUE(int_queue.Pop(result));
    EXPECT_EQ(expected, result);
  }
}

// 测试SwapQueue功能
TEST_F(BlockingQueueTest, SwapQueueFunctionality) {
  int result = 0;

  // 添加元素到生产者队列
  int_queue.Push(10);
  int_queue.Push(20);

  // Pop操作会触发SwapQueue
  EXPECT_TRUE(int_queue.Pop(result));
  EXPECT_EQ(10, result);

  EXPECT_TRUE(int_queue.Pop(result));
  EXPECT_EQ(20, result);
}

// 测试阻塞行为
TEST_F(BlockingQueueTest, BlockingBehavior) {
  std::atomic<bool> threadStarted{false};
  std::atomic<bool> popCompleted{false};
  int result = 0;

  // 创建消费者线程
  std::thread consumer([&]() {
    threadStarted = true;
    EXPECT_TRUE(int_queue.Pop(result));  // 这会阻塞直到有元素
    popCompleted = true;
  });

  // 等待线程启动
  while (!threadStarted) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // 短暂等待确认线程确实在阻塞
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_FALSE(popCompleted);

  // 添加元素应该解除阻塞
  int_queue.Push(100);

  // 等待Pop完成
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_TRUE(popCompleted);
  EXPECT_EQ(100, result);

  consumer.join();
}

// 测试Cancel功能
TEST_F(BlockingQueueTest, CancelOperation) {
  std::atomic<bool> threadStarted{false};
  std::atomic<bool> popResult{true};

  // 创建消费者线程
  std::thread consumer([&]() {
    threadStarted = true;
    int result = 0;
    popResult = int_queue.Pop(result);  // 这会阻塞直到Cancel
  });

  // 等待线程启动
  while (!threadStarted) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // 短暂等待
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // 执行Cancel
  int_queue.Cancel();

  // 等待线程结束
  consumer.join();

  // Cancel后Pop应该返回false
  EXPECT_FALSE(popResult);
}

// 测试非阻塞模式
TEST_F(BlockingQueueTest, NonBlockingMode) {
  BlockingQueue<int> nonBlockingQueue(true);  // 非阻塞模式
  int result = 0;

  // 在非阻塞模式下，空队列Pop应该立即返回false
  EXPECT_FALSE(nonBlockingQueue.Pop(result));

  // Push仍然应该工作
  nonBlockingQueue.Push(42);
  EXPECT_TRUE(nonBlockingQueue.Pop(result));
  EXPECT_EQ(42, result);
}

// 测试线程安全性
TEST_F(BlockingQueueTest, ThreadSafety) {
  const int numProducers = 2;
  const int numConsumers = 2;
  const int elementsPerProducer = 1000;
  const int totalElements = numProducers * elementsPerProducer;

  std::vector<std::thread> producers;
  std::vector<std::thread> consumers;
  std::atomic<int> consumedCount{0};
  std::atomic<int> producedCount{0};

  // 创建生产者线程
  for (int i = 0; i < numProducers; ++i) {
    producers.emplace_back([&, i]() {
      for (int j = 0; j < elementsPerProducer; ++j) {
        int value = i * elementsPerProducer + j;
        int_queue.Push(value);
        producedCount++;
      }
    });
  }

  // 创建消费者线程
  for (int i = 0; i < numConsumers; ++i) {
    consumers.emplace_back([&]() {
      int value = 0;
      while (consumedCount < totalElements) {
        if (int_queue.Pop(value)) {
          consumedCount++;
          // 验证值的范围
          EXPECT_GE(value, 0);
          EXPECT_LT(value, totalElements);
        }
      }
    });
  }

  // 等待所有生产者完成
  for (auto& producer : producers) {
    producer.join();
  }
  int_queue.Cancel();

  // 等待所有消费者完成
  for (auto& consumer : consumers) {
    consumer.join();
  }

  // 验证所有元素都被处理
  EXPECT_EQ(totalElements, producedCount);
  EXPECT_EQ(totalElements, consumedCount);

  // 队列应该是空的
}
