#ifndef CURRENT_THREAD_H_
#define CURRENT_THREAD_H_

#include <string>

namespace ywh {
namespace utils {
namespace current_thread {

extern thread_local int t_thread_cached_tid;

void SetTid(const int id);

/**
 * @brief 初始化线程局部存储的线程ID信息
 * @note 非线程安全，应在每个线程首次访问tid前调用
 */
void CacheTid();

inline const int GetTid() {
  if (__builtin_expect(t_thread_cached_tid == 0, 0)) CacheTid();
  return t_thread_cached_tid;
}

}  // namespace current_thread
}  // namespace utils
}  // namespace ywh

#endif  // CURRENT_THREAD_H_