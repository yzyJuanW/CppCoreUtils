#include "utils/current_thread.h"
#include <unistd.h>
#include <sys/syscall.h>
using namespace ywh::utils;
thread_local int current_thread::t_thread_cached_tid = 0;


void current_thread::CacheTid() {
  if (current_thread::t_thread_cached_tid == 0) {
    current_thread::t_thread_cached_tid = static_cast<pid_t>(::syscall(SYS_gettid));
  }
}

void current_thread::SetTid(const int id) {
  current_thread::t_thread_cached_tid = id;
}