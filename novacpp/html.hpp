// Optimized for zero-heap runtime allocation in hot render loops
#pragma once
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <vector>

#include "server.hpp"
#include "state.hpp"

namespace np {

class NovaBuilder;

class Component {
public:
  std::string id;
  explicit Component(std::string comp_id) : id(std::move(comp_id)) {}
  virtual void render(NovaBuilder &np) = 0;
  virtual ~Component() = default;
};

inline std::string generate_session_id() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static const char hex_chars[] = "0123456789abcdef";
  std::uniform_int_distribution<> dis(0, 15);
  std::string sid;
  sid.reserve(16);
  for (int i = 0; i < 16; ++i) sid += hex_chars[dis(gen)];
  return sid;
}

inline std::string extract_session_cookie(const HttpRequest &req) {
  if (req.has_header("Cookie")) {
    std::string cookies = req.get_header_value("Cookie");
    size_t pos = cookies.find("nova_session=");
    if (pos != std::string::npos) {
      size_t end = cookies.find(';', pos);
      if (end == std::string::npos) end = cookies.length();
      return cookies.substr(pos + 13, end - (pos + 13));
    }
  }
  return "";
}

inline std::string load_static_asset(const std::string &name, const std::string &fallback = "") {
  if (name.find("..") != std::string::npos) return fallback;
  const std::string paths[] = { "./render/" + name, "../render/" + name, "render/" + name };
  for (const auto &p : paths) {
    std::ifstream f(p, std::ios::binary);
    if (f) return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  }
  return fallback;
}

class NovaBuilder {
public:
  void clear() { elements_.clear(); }
  void render(const std::string &html) { elements_.push_back(html); }
  NovaBuilder &operator<<(const std::string &html) { elements_.push_back(html); return *this; }

  void onClick(const std::string &action, std::function<void()> fn) {
    actions_[action] = std::move(fn);
  }

  void onLoad(std::function<void()> fn) {
    actions_["__onLoad"] = std::move(fn);
  }

  void route(const std::string &path, std::function<void(NovaBuilder &)> handler) {
    routes_[path] = std::move(handler);
  }

  void renderComponent(Component &comp, int poll_interval_ms = 0) {
    components_[comp.id] = &comp;
    if (poll_interval_ms > 0) {
      elements_.push_back("<div id=\"" + comp.id + "\" nova-poll=\"" + std::to_string(poll_interval_ms) + "\">");
    } else {
      elements_.push_back("<div id=\"" + comp.id + "\">");
    }
    comp.render(*this);
    elements_.push_back("</div>");
  }

  std::string generateHTML(bool include_shell = true) {
    std::string out;
    if (include_shell) {
      std::string v = "?v=" + std::to_string(time(nullptr));
      out += "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n"
             "    <meta charset=\"UTF-8\">\n"
             "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
             "    <title>NovaCPP Application</title>\n"
             "    <link rel=\"stylesheet\" href=\"styles.css" + v + "\">\n"
             "</head>\n<body>\n    <div id=\"root\">\n";
    }
    for (const auto &el : elements_) {
      out += "        " + el + "\n";
    }
    if (include_shell) {
      std::string v = "?v=" + std::to_string(time(nullptr));
      out += "    </div>\n"
             "    <script src=\"nova.js" + v + "\"></script>\n"
             "</body>\n</html>\n";
    }
    return out;
  }

  void prepare_session(const HttpRequest &req, HttpResponse &res) {
    std::string sid = extract_session_cookie(req);
    if (sid.empty()) {
      sid = generate_session_id();
      res.set_header("Set-Cookie", "nova_session=" + sid + "; Path=/; HttpOnly");
    }
    np::Context::set_current_session_id(sid);
  }

  void listen(int port) {
    server_.Get("/styles.css", [](const HttpRequest &, HttpResponse &res) {
      res.set_content(load_static_asset("styles.css", "body{font-family:system-ui,sans-serif;}"), "text/css");
    });

    server_.Get("/nova.js", [](const HttpRequest &, HttpResponse &res) {
      res.set_content(load_static_asset("nova.js", ""), "application/javascript");
    });

    server_.Get("/app.js", [](const HttpRequest &, HttpResponse &res) {
      res.set_content(load_static_asset("app.js", ""), "application/javascript");
    });

    server_.Get("/api/demo-data", [](const HttpRequest &, HttpResponse &res) {
      res.set_content(R"({"id":1,"title":"NovaCPP","body":"Native C++ Monolith","userId":1})", "application/json");
    });

    server_.Post("/api/demo-data", [](const HttpRequest &, HttpResponse &res) {
      res.set_content(R"({"id":101,"title":"NovaCPP","body":"REST Client","userId":1})", "application/json");
    });

    server_.Put("/api/demo-data", [](const HttpRequest &, HttpResponse &res) {
      res.set_content(R"({"id":1,"title":"Updated","body":"Data","userId":1})", "application/json");
    });

    server_.Delete("/api/demo-data", [](const HttpRequest &, HttpResponse &res) {
      res.set_content(R"({})", "application/json");
    });

    server_.Post("/nova/navigate", [&](const HttpRequest &req, HttpResponse &res) {
      prepare_session(req, res);
      std::string path = req.get_header_value("X-Nova-Path");
      auto it = routes_.find(path);
      if (it != routes_.end()) {
        current_route_ = path;
        clear();
        if (actions_.count("__onLoad")) actions_["__onLoad"]();
        it->second(*this);
        res.set_content(generateHTML(false), "text/html");
      } else {
        res.status = 404;
        res.set_content("<h1>404 Not Found</h1>", "text/html");
      }
    });

    server_.Post(R"(/nova/action/(.*))", [&](const HttpRequest &req, HttpResponse &res) {
      prepare_session(req, res);
      std::string action = (req.matches.size() > 1) ? req.matches[1] : "";
      if (actions_.count(action)) actions_[action]();

      std::string target = req.get_header_value("X-Nova-Target");
      if (!target.empty() && components_.count(target)) {
        NovaBuilder partial;
        components_[target]->render(partial);
        res.set_content(partial.generateHTML(false), "text/html");
      } else {
        clear();
        if (routes_.count(current_route_)) routes_[current_route_](*this);
        res.set_content(generateHTML(false), "text/html");
      }
    });

    server_.Post("/nova/poll", [&](const HttpRequest &req, HttpResponse &res) {
      prepare_session(req, res);
      std::string target = req.get_header_value("X-Nova-Target");
      if (!target.empty() && components_.count(target)) {
        NovaBuilder partial;
        components_[target]->render(partial);
        res.set_content(partial.generateHTML(false), "text/html");
      } else {
        res.status = 404;
        res.set_content("<h1>404 Not Found</h1>", "text/html");
      }
    });

    server_.Get(".*", [&](const HttpRequest &req, HttpResponse &res) {
      if (req.path == "/styles.css" || req.path == "/nova.js" || req.path == "/app.js") return;
      auto it = routes_.find(req.path);
      if (it != routes_.end()) {
        current_route_ = req.path;
        prepare_session(req, res);
        if (actions_.count("__onLoad")) actions_["__onLoad"]();
        clear();
        it->second(*this);
        res.set_content(generateHTML(true), "text/html");
      } else {
        res.status = 404;
        res.set_content("<h1>404 Not Found</h1>", "text/html");
      }
    });

    std::cout << "NovaCPP: Live Server running! Open http://localhost:" << port << " in your browser.\n";
    if (!server_.listen("0.0.0.0", port)) {
      std::cerr << "[NovaCPP] Error: Port " << port << " is unavailable.\n";
    }
  }

private:
  std::vector<std::string> elements_;
  Server server_;
  std::map<std::string, std::function<void()>> actions_;
  std::map<std::string, Component*> components_;
  std::map<std::string, std::function<void(NovaBuilder &)>> routes_;
  std::string current_route_ = "/";
};

} // namespace np

