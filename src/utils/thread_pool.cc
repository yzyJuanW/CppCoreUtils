#include "utils/thread_pool.hpp"

#include "utils/current_thread.h"

using namespace ywh::utils;

ywh::utils::ThreadPool::ThreadPool(int pool_size) : m_pool_size(pool_size) {
  m_task_workers.reserve(m_pool_size);
  for (int i = 0; i < m_pool_size; i++) {
    m_task_workers.emplace_back([this, i]() {
      current_thread::SetTid(i + 1);
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