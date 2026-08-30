#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
typedef SOCKET sock_handle_t;
#define CLOSE_SOCK(s) closesocket(s)
#define BAD_SOCK(s) ((s) == INVALID_SOCKET)
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int sock_handle_t;
#define CLOSE_SOCK(s) ::close(s)
#define BAD_SOCK(s) ((s) < 0)
#endif

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <string>

namespace np {

inline void init_network() {
#ifdef _WIN32
  static bool ready = false;
  if (!ready) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    ready = true;
  }
#endif
}

inline bool parse_url(const std::string &url, std::string &scheme,
                      std::string &host, int &port, std::string &path) {
  if (url.empty()) return false;

  size_t proto = url.find("://");
  std::string rem = (proto != std::string::npos) ? url.substr(proto + 3) : url;
  scheme = (proto != std::string::npos) ? url.substr(0, proto) : "http";
  std::transform(scheme.begin(), scheme.end(), scheme.begin(), [](unsigned char c) { return std::tolower(c); });

  if (rem.empty()) return false;

  size_t slash = rem.find('/');
  std::string host_part = (slash != std::string::npos) ? rem.substr(0, slash) : rem;
  path = (slash != std::string::npos) ? rem.substr(slash) : "/";
  if (path.empty()) path = "/";

  size_t colon = host_part.find(':');
  if (colon != std::string::npos) {
    host = host_part.substr(0, colon);
    try {
      port = std::stoi(host_part.substr(colon + 1));
    } catch (...) {
      return false;
    }
  } else {
    host = host_part;
    port = (scheme == "https") ? 443 : 80;
  }

  return !host.empty() && port > 0 && port <= 65535;
}

inline std::string fetch(const std::string &url,
                         const std::string &method = "GET",
                         const std::string &body = "",
                         const std::string &headers = "") {
  init_network();

  std::string scheme, host, path;
  int port = 80;
  if (!parse_url(url, scheme, host, port, path)) {
    return "{\"error\":\"Invalid URL\"}";
  }
  if (scheme == "https") {
    return "{\"error\":\"Zero-dependency NovaCPP supports native HTTP\"}";
  }

  addrinfo hints, *res = nullptr;
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  std::string port_str = std::to_string(port);
  if (getaddrinfo(host.c_str(), port_str.c_str(), &hints, &res) != 0 || !res) {
    return "{\"error\":\"DNS resolution failed\"}";
  }

  sock_handle_t s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (BAD_SOCK(s)) {
    freeaddrinfo(res);
    return "{\"error\":\"Socket creation failed\"}";
  }

  if (connect(s, res->ai_addr, static_cast<socklen_t>(res->ai_addrlen)) < 0) {
    CLOSE_SOCK(s);
    freeaddrinfo(res);
    return "{\"error\":\"Connection failed\"}";
  }
  freeaddrinfo(res);

  std::ostringstream req;
  req << (method.empty() ? "GET" : method) << " " << path << " HTTP/1.1\r\n"
      << "Host: " << host << ":" << port << "\r\n"
      << "User-Agent: NovaCPP\r\n";

  if (!body.empty()) {
    req << "Content-Length: " << body.size() << "\r\n";
  }
  if (!headers.empty()) {
    req << headers;
    if (headers.size() < 2 || headers.substr(headers.size() - 2) != "\r\n") req << "\r\n";
  } else {
    req << "\r\n";
  }
  if (!body.empty()) req << body;

  std::string req_str = req.str();
  if (send(s, req_str.c_str(), static_cast<int>(req_str.size()), 0) < 0) {
    CLOSE_SOCK(s);
    return "{\"error\":\"Send failed\"}";
  }

  std::string resp;
  char buf[4096];
  int n = 0;
  while ((n = recv(s, buf, sizeof(buf), 0)) > 0) {
    resp.append(buf, n);
  }
  CLOSE_SOCK(s);

  if (resp.empty()) return "{\"error\":\"Empty response\"}";

  size_t body_pos = resp.find("\r\n\r\n");
  return (body_pos != std::string::npos) ? resp.substr(body_pos + 4) : resp;
}

} // namespace np
