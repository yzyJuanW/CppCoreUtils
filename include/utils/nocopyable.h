#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

namespace ywh {
namespace utils {

class NonCopyable {
 public:
  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;

 protected:
  NonCopyable() = default;
  ~NonCopyable() = default;
};
}  // namespace utils
}  // namespace ywh

#endif  // NONCOPYABLE_H