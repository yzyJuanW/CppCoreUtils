#ifndef SINGLETON_H
#define SINGLETON_H

#include "utils/nocopyable.h"

namespace ywh {
namespace utils {
template <typename T>
class Singleton : NonCopyable {
 public:
  static T* Instance() {
    static T instance;
    return &instance;
  }

 protected:
  Singleton() = default;
  virtual ~Singleton() = default;

 private:
  Singleton(Singleton&&) = delete;
  Singleton& operator=(Singleton&&) = delete;
};
}  // namespace utils
}  // namespace ywh

/**
// use case
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
 */

#endif  // SINGLETON_H
