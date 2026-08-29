#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET sock_t;
#define CLOSE_S(s) closesocket(s)
#define BAD_S(s) ((s) == INVALID_SOCKET)

namespace np {
class Thread {
  HANDLE h_ = NULL;
  struct Task { std::function<void()> fn; };
  static DWORD WINAPI run(LPVOID p) {
    auto *t = static_cast<Task*>(p);
    t->fn();
    delete t;
    return 0;
  }
public:
  Thread() = default;
  template <typename F> explicit Thread(F f) {
    auto *task = new Task();
    task->fn = f;
    h_ = CreateThread(NULL, 0, run, task, 0, NULL);
  }
  void detach() { if (h_) { CloseHandle(h_); h_ = NULL; } }
  void join() { if (h_) { WaitForSingleObject(h_, INFINITE); CloseHandle(h_); h_ = NULL; } }
  ~Thread() { if (h_) CloseHandle(h_); }
};
} // namespace np

#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
typedef int sock_t;
#define CLOSE_S(s) ::close(s)
#define BAD_S(s) ((s) < 0)

namespace np {
using Thread = std::thread;
}
#endif

namespace np {

inline void init_server_sockets() {
#ifdef _WIN32
  static bool ready = false;
  if (!ready) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    ready = true;
  }
#endif
}

inline std::string to_lower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
  return s;
}

struct HttpRequest {
  std::string method;
  std::string path;
  std::string query;
  std::map<std::string, std::string> headers;
  std::string body;
  std::vector<std::string> matches;

  bool has_header(const std::string &name) const {
    std::string target = to_lower(name);
    for (const auto &kv : headers) {
      if (to_lower(kv.first) == target) return true;
    }
    return false;
  }

  std::string get_header_value(const std::string &name) const {
    std::string target = to_lower(name);
    for (const auto &kv : headers) {
      if (to_lower(kv.first) == target) return kv.second;
    }
    return "";
  }
};

struct HttpResponse {
  int status = 200;
  std::string status_text = "OK";
  std::map<std::string, std::string> headers;
  std::string body;

  void set_content(const std::string &content, const std::string &type) {
    body = content;
    headers["Content-Type"] = type;
  }

  void set_header(const std::string &k, const std::string &v) {
    headers[k] = v;
  }

  std::string to_string() const {
    std::ostringstream ss;
    ss << "HTTP/1.1 " << status << " " << (status_text.empty() ? "OK" : status_text) << "\r\n";
    bool has_len = false;
    for (const auto &kv : headers) {
      ss << kv.first << ": " << kv.second << "\r\n";
      if (to_lower(kv.first) == "content-length") has_len = true;
    }
    if (!has_len) {
      ss << "Content-Length: " << body.size() << "\r\n";
    }
    ss << "Connection: close\r\n\r\n" << body;
    return ss.str();
  }
};

class Server {
public:
  using Handler = std::function<void(const HttpRequest &, HttpResponse &)>;

  struct Route {
    std::string method;
    std::string pattern;
    Handler handler;
    bool is_action = false;
    bool is_wildcard = false;
  };

  void Get(const std::string &path, Handler fn)    { add_route("GET", path, fn); }
  void Post(const std::string &path, Handler fn)   { add_route("POST", path, fn); }
  void Put(const std::string &path, Handler fn)    { add_route("PUT", path, fn); }
  void Delete(const std::string &path, Handler fn) { add_route("DELETE", path, fn); }

  bool listen(const std::string &host, int port) {
    init_server_sockets();

    sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
    if (BAD_S(srv)) return false;

    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (host.empty() || host == "0.0.0.0") {
      addr.sin_addr.s_addr = INADDR_ANY;
    } else {
#ifdef _WIN32
      addr.sin_addr.s_addr = inet_addr(host.c_str());
#else
      inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
#endif
    }

    if (bind(srv, (sockaddr *)&addr, sizeof(addr)) < 0 || ::listen(srv, 128) < 0) {
      CLOSE_S(srv);
      return false;
    }

    running_ = true;
    while (running_) {
      sockaddr_in client_addr;
      socklen_t len = sizeof(client_addr);
      sock_t client = accept(srv, (sockaddr *)&client_addr, &len);
      if (BAD_S(client)) {
        if (!running_) break;
        continue;
      }
      np::Thread([this, client]() { this->handle_client(client); }).detach();
    }

    CLOSE_S(srv);
    return true;
  }

  void stop() { running_ = false; }

private:
  std::vector<Route> routes_;
  bool running_ = false;

  void add_route(const std::string &method, const std::string &pattern, Handler fn) {
    Route r;
    r.method = method;
    r.pattern = pattern;
    r.handler = fn;
    r.is_action = (pattern.find("/nova/action/") != std::string::npos || pattern.find("(.*)") != std::string::npos);
    r.is_wildcard = (pattern == ".*" || pattern == "*");
    routes_.push_back(r);
  }

  void handle_client(sock_t client) {
    std::vector<char> buf(65536);
    int n = recv(client, buf.data(), static_cast<int>(buf.size() - 1), 0);
    if (n <= 0) {
      CLOSE_S(client);
      return;
    }
    buf[n] = '\0';

    HttpRequest req;
    parse_request(std::string(buf.data(), n), req, client);

    HttpResponse res;
    bool matched = false;

    for (const auto &r : routes_) {
      if (r.method != req.method) continue;

      if (r.is_wildcard) {
        req.matches = {req.path};
        r.handler(req, res);
        matched = true;
        break;
      }

      if (r.is_action) {
        std::string prefix = "/nova/action/";
        if (req.path.rfind(prefix, 0) == 0) {
          req.matches = {req.path, req.path.substr(prefix.length())};
          r.handler(req, res);
          matched = true;
          break;
        }
      }

      if (r.pattern == req.path) {
        req.matches = {req.path};
        r.handler(req, res);
        matched = true;
        break;
      }
    }

    if (!matched) {
      res.status = 404;
      res.set_content("<h1>404 Not Found</h1>", "text/html");
    }

    std::string out = res.to_string();
    send(client, out.c_str(), static_cast<int>(out.size()), 0);
    CLOSE_S(client);
  }

  void parse_request(const std::string &raw, HttpRequest &req, sock_t client) {
    std::istringstream stream(raw);
    std::string line;
    if (!std::getline(stream, line)) return;
    if (!line.empty() && line.back() == '\r') line.pop_back();

    std::istringstream lstream(line);
    std::string full_path;
    lstream >> req.method >> full_path;

    size_t q = full_path.find('?');
    req.path = (q != std::string::npos) ? full_path.substr(0, q) : full_path;
    req.query = (q != std::string::npos) ? full_path.substr(q + 1) : "";

    size_t content_len = 0;
    while (std::getline(stream, line)) {
      if (!line.empty() && line.back() == '\r') line.pop_back();
      if (line.empty()) break;

      size_t colon = line.find(':');
      if (colon != std::string::npos) {
        std::string k = line.substr(0, colon);
        std::string v = line.substr(colon + 1);
        size_t first = v.find_first_not_of(" \t");
        size_t last = v.find_last_not_of(" \t");
        if (first != std::string::npos && last != std::string::npos) {
          v = v.substr(first, last - first + 1);
        }
        req.headers[k] = v;
        if (to_lower(k) == "content-length") {
          try { content_len = std::stoul(v); } catch (...) {}
        }
      }
    }

    size_t header_end = raw.find("\r\n\r\n");
    if (header_end != std::string::npos) {
      size_t body_start = header_end + 4;
      if (raw.size() >= body_start) req.body = raw.substr(body_start);
      while (req.body.size() < content_len) {
        std::vector<char> b(8192);
        int r = recv(client, b.data(), static_cast<int>(b.size()), 0);
        if (r <= 0) break;
        req.body.append(b.data(), r);
      }
      if (req.body.size() > content_len && content_len > 0) {
        req.body = req.body.substr(0, content_len);
      }
    }
  }
};

} // namespace np
