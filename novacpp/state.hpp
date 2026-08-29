#pragma once
#include <string>
#include <unordered_map>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace np {
class Mutex {
  CRITICAL_SECTION cs;
public:
  Mutex() { InitializeCriticalSection(&cs); }
  ~Mutex() { DeleteCriticalSection(&cs); }
  void lock() { EnterCriticalSection(&cs); }
  void unlock() { LeaveCriticalSection(&cs); }
};

class LockGuard {
  Mutex &m_;
public:
  explicit LockGuard(Mutex &m) : m_(m) { m_.lock(); }
  ~LockGuard() { m_.unlock(); }
};

namespace Context {
inline DWORD tls_index() {
  static DWORD idx = TlsAlloc();
  return idx;
}

inline std::string get_current_session_id() {
  void *p = TlsGetValue(tls_index());
  return p ? std::string(static_cast<const char*>(p)) : "default";
}

inline void set_current_session_id(const std::string &sid) {
  char *old = static_cast<char*>(TlsGetValue(tls_index()));
  if (old) free(old);
  char *buf = static_cast<char*>(malloc(sid.size() + 1));
  if (buf) {
    memcpy(buf, sid.c_str(), sid.size() + 1);
    TlsSetValue(tls_index(), buf);
  }
}
} // namespace Context
} // namespace np

#else
#include <mutex>

namespace np {
using Mutex = std::mutex;
using LockGuard = std::lock_guard<std::mutex>;

namespace Context {
inline std::string &session_storage() {
  static thread_local std::string sid = "default";
  return sid;
}

inline std::string get_current_session_id() {
  return session_storage();
}

inline void set_current_session_id(const std::string &sid) {
  session_storage() = sid;
}
} // namespace Context
} // namespace np
#endif

namespace np {
template <typename T>
class State {
public:
  explicit State(T initial) : initial_(initial) {}

  T get() {
    LockGuard lock(mtx_);
    std::string sid = Context::get_current_session_id();
    auto it = store_.find(sid);
    if (it != store_.end()) return it->second;
    store_[sid] = initial_;
    return initial_;
  }

  void set(const T &val) {
    LockGuard lock(mtx_);
    store_[Context::get_current_session_id()] = val;
  }

  State &operator=(const T &val) {
    set(val);
    return *this;
  }

  operator T() { return get(); }

private:
  T initial_;
  std::unordered_map<std::string, T> store_;
  mutable Mutex mtx_;
};
} // namespace np
