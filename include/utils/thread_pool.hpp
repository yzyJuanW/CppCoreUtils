#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <functional>
#include <thread>
#include <vector>

#include "utils/blocking_queue.hpp"
#include "utils/nocopyable.h"

namespace ywh {
namespace utils {

class ThreadPool : NonCopyable {
 public:
  using TaskFunc = std::function<void()>;
  explicit ThreadPool(const int pool_size = 8);
  ~ThreadPool();

  template <typename Func, typename... Args>
  void Enqueue(Func&& func, Args&&... args);

 private:
  BlockingQueue<TaskFunc> m_tasks_queue;
  std::vector<std::thread> m_task_workers;
};

template <typename Func, typename... Args>
inline void ThreadPool::Enqueue(Func&& func, Args&&... args) {
  auto tasks = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
  m_tasks_queue.Push(tasks);
}

}  // namespace utils
}  // namespace ywh

#endif  // THREAD_POOL_HPP