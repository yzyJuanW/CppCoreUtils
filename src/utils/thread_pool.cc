#include "utils/thread_pool.hpp"

using namespace ywh::utils;

ywh::utils::ThreadPool::ThreadPool(int pool_size) {
  m_task_workers.reserve(pool_size);
  while (pool_size--) {
    m_task_workers.emplace_back([this]() {
      TaskFunc task_func;
      while (m_tasks_queue.Pop(task_func)) {
        task_func();
      }
      return;
    });
  }
}
ywh::utils::ThreadPool::~ThreadPool() {
  m_tasks_queue.Cancel();
  for (auto& task_worker : m_task_workers) {
    if (task_worker.joinable()) {
      task_worker.join();
    }
  }
}