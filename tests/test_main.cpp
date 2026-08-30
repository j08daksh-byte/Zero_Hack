#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#endif

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>

#include "novacpp/server.hpp"
#include "novacpp/state.hpp"
#include "novacpp/fetch.hpp"
#include "novacpp/html.hpp"

static int g_passed = 0;
static int g_failed = 0;

#define TEST(fn) \
  do { \
    std::cout << "[TEST] " << #fn << "... " << std::flush; \
    if (fn()) { \
      std::cout << "PASSED\n"; \
      g_passed++; \
    } else { \
      std::cout << "FAILED\n"; \
      g_failed++; \
    } \
  } while (0)

#define CHECK(cond) \
  if (!(cond)) { \
    std::cerr << " FAIL (" << #cond << ") on line " << __LINE__ << "\n"; \
    return false; \
  }

#define CHECK_EQ(a, b) \
  if ((a) != (b)) { \
    std::cerr << " FAIL (" << #a << " == " << #b << ") on line " << __LINE__ << "\n"; \
    return false; \
  }

bool test_url_parsing() {
  std::string scheme, host, path;
  int port = 0;

  CHECK(np::parse_url("http://example.com", scheme, host, port, path));
  CHECK_EQ(scheme, "http");
  CHECK_EQ(host, "example.com");
  CHECK_EQ(port, 80);
  CHECK_EQ(path, "/");

  CHECK(np::parse_url("http://127.0.0.1:8080/api/data?id=42", scheme, host, port, path));
  CHECK_EQ(host, "127.0.0.1");
  CHECK_EQ(port, 8080);
  CHECK_EQ(path, "/api/data?id=42");

  CHECK(!np::parse_url("", scheme, host, port, path));
  CHECK(!np::parse_url("http://:80", scheme, host, port, path));
  return true;
}

bool test_port_parsing() {
  auto parse_port = [](const std::string &val) -> int {
    int p = 8080;
    try {
      int parsed = std::stoi(val);
      if (parsed > 0 && parsed <= 65535) p = parsed;
    } catch (...) {}
    return p;
  };

  CHECK_EQ(parse_port("3000"), 3000);
  CHECK_EQ(parse_port("8080"), 8080);
  CHECK_EQ(parse_port("invalid"), 8080);
  CHECK_EQ(parse_port("-1"), 8080);
  CHECK_EQ(parse_port("99999"), 8080);
  return true;
}

bool test_state_isolation() {
  np::State<int> counter(10);

  np::Context::set_current_session_id("sess_A");
  CHECK_EQ(counter.get(), 10);
  counter = 42;
  CHECK_EQ(counter.get(), 42);

  np::Context::set_current_session_id("sess_B");
  CHECK_EQ(counter.get(), 10);
  counter = 100;
  CHECK_EQ(counter.get(), 100);

  np::Context::set_current_session_id("sess_A");
  CHECK_EQ(counter.get(), 42);
  return true;
}

bool test_state_concurrency() {
  np::State<int> counter(0);
  const int NUM_THREADS = 8;
  const int ITERS = 50;

  std::vector<np::Thread*> threads;
  for (int i = 0; i < NUM_THREADS; ++i) {
    threads.push_back(new np::Thread([&counter, i, ITERS]() {
      np::Context::set_current_session_id("worker_" + std::to_string(i));
      for (int j = 0; j < ITERS; ++j) {
        counter = counter.get() + 1;
      }
    }));
  }

  for (auto *t : threads) {
    t->join();
    delete t;
  }

  for (int i = 0; i < NUM_THREADS; ++i) {
    np::Context::set_current_session_id("worker_" + std::to_string(i));
    CHECK_EQ(counter.get(), ITERS);
  }
  return true;
}

bool test_static_file_security() {
  std::string res = np::load_static_asset("../secret.txt", "BLOCKED");
  CHECK_EQ(res, "BLOCKED");
  return true;
}

bool test_server_and_fetch_integration() {
  const int TEST_PORT = 8998;

  np::Thread server([TEST_PORT]() {
    np::NovaBuilder b;
    b.route("/", [](np::NovaBuilder &nb) { nb << "<h1>Nova Home</h1>"; });
    b.listen(TEST_PORT);
  });
  server.detach();

#ifdef _WIN32
  Sleep(250);
#else
  usleep(250000);
#endif

  std::string home = np::fetch("http://127.0.0.1:" + std::to_string(TEST_PORT) + "/", "GET");
  CHECK(home.find("Nova Home") != std::string::npos);

  std::string demo = np::fetch("http://127.0.0.1:" + std::to_string(TEST_PORT) + "/api/demo-data", "GET");
  CHECK(demo.find("NovaCPP") != std::string::npos);

  std::string not_found = np::fetch("http://127.0.0.1:" + std::to_string(TEST_PORT) + "/bad-route", "GET");
  CHECK(not_found.find("404 Not Found") != std::string::npos);
  return true;
}

bool test_concurrent_http_requests() {
  const int TEST_PORT = 8998;
  const int NUM_REQS = 8;
  std::vector<np::Thread*> clients;
  std::vector<bool> ok(NUM_REQS, false);

  for (int i = 0; i < NUM_REQS; ++i) {
    clients.push_back(new np::Thread([&ok, i, TEST_PORT]() {
      std::string res = np::fetch("http://127.0.0.1:" + std::to_string(TEST_PORT) + "/api/demo-data", "GET");
      if (res.find("NovaCPP") != std::string::npos) ok[i] = true;
    }));
  }

  for (auto *c : clients) {
    c->join();
    delete c;
  }

  for (int i = 0; i < NUM_REQS; ++i) {
    CHECK(ok[i]);
  }
  return true;
}

int main() {
  std::cout << "NovaCPP Zero-Dependency Test Suite\n";
  std::cout << "-----------------------------------\n";

  TEST(test_url_parsing);
  TEST(test_port_parsing);
  TEST(test_state_isolation);
  TEST(test_state_concurrency);
  TEST(test_static_file_security);
  TEST(test_server_and_fetch_integration);
  TEST(test_concurrent_http_requests);

  std::cout << "-----------------------------------\n";
  std::cout << "Results: " << g_passed << " Passed, " << g_failed << " Failed\n";
  return g_failed == 0 ? 0 : 1;
}
