#ifndef BLOCKING_QUEUE_HPP
#define BLOCKING_QUEUE_HPP

#include <condition_variable>
#include <mutex>
#include <queue>

namespace ywh {
namespace utils {
/**
 * @brief 线程安全的阻塞队列模板类
 * @tparam Element 队列中元素的类型
 * @details 使用双队列设计减少锁竞争，支持阻塞弹出和非阻塞模式切换
 */
template <typename Element>
class BlockingQueue {
 public:
  /**
   * @brief 构造函数
   * @param nonblock 是否启用非阻塞模式 (默认false，即阻塞模式)
   */
  BlockingQueue(bool nonblock = false) : m_nonblock(nonblock) {}

  /**
   * @brief 将元素推入队列
   * @param element 要推入队列的元素常量引用
   * @details 此操作会通知一个等待的消费者线程
   */
  void Push(const Element& element);

  /**
   * @brief 从队列中弹出元素
   * @param ret_element 用于接收弹出元素的引用
   * @return 成功弹出返回true，队列为空且处于非阻塞模式返回false
   * @details 在阻塞模式下，此方法会等待直到队列不为空
   */
  bool Pop(Element& ret_element);

  /**
   * @brief 取消所有阻塞操作
   * @details 将所有等待中的线程唤醒并设置非阻塞模式，通常用于关闭队列时
   */
  void Cancel();

 private:
  /**
   * @brief 交换生产者和消费者队列
   * @return 交换后消费者队列中的元素数量
   * @details 内部方法，用于减少锁竞争，在持有生产者锁的情况下等待条件变量
   */
  int SwapQueue();

 private:
  bool m_nonblock;                       ///< 是否非阻塞模式标志
  std::queue<Element> m_producer_queue;  ///< 生产者队列，用于接收新元素
  std::queue<Element> m_consumer_queue;  ///< 消费者队列，用于提供元素给消费者
  std::mutex m_producer_mutex;           ///< 保护生产者队列的互斥锁
  std::mutex m_consumer_mutex;           ///< 保护消费者队列的互斥锁
  std::condition_variable m_not_empty;   ///< 条件变量，用于通知队列非空状态
};

// 模板类方法实现

/**
 * @brief 将元素推入队列
 * @tparam Element 队列元素类型
 * @param element 要推入队列的元素
 * @details 此操作是线程安全的，会通知一个等待的消费者线程
 */
template <typename Element>
inline void BlockingQueue<Element>::Push(const Element& element) {
  std::lock_guard<std::mutex> lock(m_producer_mutex);
  m_producer_queue.push(element);
  m_not_empty.notify_one();  // 通知一个等待的消费者
}

/**
 * @brief 从队列中弹出元素
 * @tparam Element 队列元素类型
 * @param ret_element 用于接收弹出元素的引用
 * @return 成功弹出返回true，队列为空且处于非阻塞模式返回false
 * @details 在阻塞模式下，此方法会等待直到队列不为空
 */
template <typename Element>
inline bool BlockingQueue<Element>::Pop(Element& ret_element) {
  std::unique_lock<std::mutex> lock(m_consumer_mutex);
  if (m_consumer_queue.empty() && SwapQueue() == 0) {
    return false;
  }

  ret_element = m_consumer_queue.front();
  m_consumer_queue.pop();
  return true;
}

/**
 * @brief 取消所有阻塞操作
 * @tparam Element 队列元素类型
 * @details 将所有等待中的线程唤醒并设置非阻塞模式，通常用于关闭队列时
 */
template <typename Element>
inline void BlockingQueue<Element>::Cancel() {
  std::lock_guard<std::mutex> lock(m_producer_mutex);
  m_nonblock = true;
  m_not_empty.notify_all();  // 通知所有等待的消费者
}

/**
 * @brief 交换生产者和消费者队列
 * @tparam Element 队列元素类型
 * @return 交换后消费者队列中的元素数量
 * @details 内部方法，在持有生产者锁的情况下等待条件变量
 */
template <typename Element>
inline int BlockingQueue<Element>::SwapQueue() {
  std::unique_lock<std::mutex> lock(m_producer_mutex);
  m_not_empty.wait(lock, [this] { return !m_producer_queue.empty() || m_nonblock; });
  std::swap(m_producer_queue, m_consumer_queue);
  return m_consumer_queue.size();
}

}  // namespace utils
}  // namespace ywh

#endif  // BLOCKING_QUEUE_HPP