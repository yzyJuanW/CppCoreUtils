#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <functional>
#include <future>
#include <thread>
#include <vector>

#include "utils/blocking_queue.hpp"
#include "utils/nocopyable.h"

namespace ywh {
namespace utils {

/**
 * @class ThreadPool
 * @brief 线程池类，用于高效管理并发任务
 *
 * @details 提供任务提交接口，支持普通任务和带返回值的任务
 *          使用双队列设计减少锁竞争，支持优雅关闭
 *          继承自 NonCopyable 禁止拷贝
 */
class ThreadPool : NonCopyable {
 public:
  /// 任务函数类型定义
  using TaskFunc = std::function<void()>;

  /**
   * @brief 构造函数
   * @param pool_size 线程池大小，默认为当前系统支持的并发线程数量的一个估计值
   *
   * @note 线程池大小至少为1，若输入<=0则自动设为1
   * @throws std::system_error 如果线程创建失败
   */
  explicit ThreadPool(const int pool_size = std::thread::hardware_concurrency());

  /**
   * @brief 析构函数
   *
   * @details 执行优雅关闭：
   *          1. 设置停止标志
   *          2. 取消所有阻塞操作
   *          3. 等待所有工作线程结束
   */
  ~ThreadPool();

  /**
   * @brief 提交无返回值的任务
   * @tparam Func 可调用对象类型
   * @tparam Args 参数类型包
   * @param func 要执行的可调用对象
   * @param args 传递给 func 的参数
   *
   * @throws std::runtime_error 如果线程池已停止
   *
   * @note 这是高性能版本，无额外开销
   *       适用于不需要结果的任务
   */
  template <typename Func, typename... Args>
  void Enqueue(Func&& func, Args&&... args);

  /**
   * @brief 提交带返回值的任务
   * @tparam Func 可调用对象类型
   * @tparam Args 参数类型包
   * @param func 要执行的可调用对象
   * @param args 传递给 func 的参数
   * @return std::future<ResultType> 用于获取任务结果
   *
   * @throws std::runtime_error 如果线程池已停止
   *
   * @note 返回的 future 可用于：
   *       1. 获取任务返回值
   *       2. 捕获任务抛出的异常
   *       3. 等待任务完成
   */
  template <typename Func, typename... Args>
  auto EnqueueWithFuture(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>;

  /**
   * @brief 获取线程池大小
   * @return 当前线程池中的工作线程数量
   */
  const int ThreadPoolSize() const { return m_pool_size; }

 private:
  const int m_pool_size;                          ///< 线程池大小
  BlockingQueue<TaskFunc> m_tasks_queue;    ///< 任务队列
  std::vector<std::thread> m_task_workers;  ///< 工作线程集合
};

template <typename Func, typename... Args>
inline void ThreadPool::Enqueue(Func&& func, Args&&... args) {
  auto tasks = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
  m_tasks_queue.Push(tasks);
}

template <typename Func, typename... Args>
inline auto ThreadPool::EnqueueWithFuture(Func&& func, Args&&... args)
    -> std::future<decltype(func(args...))> {
  using RetType = decltype(func(args...));
  using PackTask = std::packaged_task<RetType()>;  // packaged_task可换成用于捕获异常的promise
  using Future = std::future<RetType>;
  auto task_ptr =
      std::make_shared<PackTask>(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
  Future ret = task_ptr->get_future();
  m_tasks_queue.Push([task_ptr] { (*task_ptr)(); });
  return ret;
}

}  // namespace utils
}  // namespace ywh

#endif  // THREAD_POOL_HPP