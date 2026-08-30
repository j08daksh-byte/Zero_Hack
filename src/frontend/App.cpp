#include <ctime>
#include <string>
#include <iostream>
#include "../../novacpp/html.hpp"
#include "../../novacpp/state.hpp"
#include "../../novacpp/fetch.hpp"

// Reactive session-isolated state
static np::State<int> g_counter(0);
static np::State<std::string> g_quote("Click any REST method below to test network round-trip.");

class NavBarComponent : public np::Component {
public:
  NavBarComponent() : Component("navbar-comp") {}

  void render(np::NovaBuilder &np) override {
    np << R"(
      <div class="bento-card col-12 navbar-card">
        <div class="nav-links">
          <a href="/" class="nav-link" nova-link>Home</a>
          <a href="/docs" class="nav-link" nova-link>Docs</a>
          <a href="/about" class="nav-link" nova-link>About</a>
        </div>
        <div class="engine-badge">NOVACPP ENGINE</div>
      </div>
    )";
  }
};

class ClockComponent : public np::Component {
public:
  ClockComponent() : Component("clock-comp") {}

  void render(np::NovaBuilder &np) override {
    time_t now = time(nullptr);
    tm *t = localtime(&now);
    char buf[64];
    strftime(buf, sizeof(buf), "%H:%M:%S", t);

    np << R"(
      <div class="bento-card col-4">
        <div class="badge badge-live">LIVE POLLING</div>
        <div>
          <div class="counter-val">)" + std::string(buf) + R"(</div>
          <div class="card-desc">C++ clock updating automatically via micro-polling.</div>
        </div>
      </div>
    )";
  }
};

class CounterComponent : public np::Component {
public:
  CounterComponent() : Component("counter-comp") {}

  void render(np::NovaBuilder &np) override {
    np << R"(
      <div class="bento-card col-4">
        <div class="badge">STATE HOOK</div>
        <div class="counter-val">)" + std::to_string(g_counter.get()) + R"(</div>
        <div class="btn-group">
          <button class="btn-secondary" nova-click="decrement" nova-target="counter-comp">- Decrease</button>
          <button class="btn-primary" nova-click="increment" nova-target="counter-comp">+ Increase</button>
        </div>
      </div>
    )";
  }
};

class RestApiComponent : public np::Component {
public:
  RestApiComponent() : Component("api-comp") {}

  void render(np::NovaBuilder &np) override {
    np << R"(
      <div class="bento-card col-4">
        <div class="badge">NATIVE HTTP CLIENT</div>
        <div class="quote-box" style="margin-bottom:0.8rem;height:100px;">
          <div class="quote-text" style="-webkit-line-clamp:4;">)" + g_quote.get() + R"(</div>
        </div>
        <div style="display:grid;grid-template-columns:1fr 1fr;gap:0.5rem;width:100%;">
          <button class="btn-secondary" nova-click="apiGet" nova-target="api-comp">GET</button>
          <button class="btn-primary" nova-click="apiPost" nova-target="api-comp">POST</button>
          <button class="btn-secondary" nova-click="apiPut" nova-target="api-comp">PUT</button>
          <button class="btn-primary" style="background:#ff3b30;" nova-click="apiDelete" nova-target="api-comp">DELETE</button>
        </div>
      </div>
    )";
  }
};

class FeatureBlock : public np::Component {
  std::string num_, title_, desc_;
public:
  FeatureBlock(std::string id, std::string num, std::string title, std::string desc)
    : Component(std::move(id)), num_(std::move(num)), title_(std::move(title)), desc_(std::move(desc)) {}

  void render(np::NovaBuilder &np) override {
    np << R"(
      <div class="bento-card col-4">
        <div class="feature-number">)" + num_ + R"(</div>
        <div>
          <h3 class="card-title">)" + title_ + R"(</h3>
          <p class="card-desc">)" + desc_ + R"(</p>
        </div>
      </div>
    )";
  }
};

class AboutComponent : public np::Component {
public:
  AboutComponent() : Component("about-comp") {}

  void render(np::NovaBuilder &np) override {
    np << R"(
      <div class="bento-card col-12 about-card">
        <div class="badge badge-indigo">FRAMEWORK ARCHITECTURE</div>
        <h1 class="about-title">About NovaCPP</h1>
        <p class="about-lead">
          NovaCPP brings component-based single page applications to standard C++ without Node.js or third-party web libraries.
        </p>
        <div class="about-pills">
          <span class="about-pill">&#9889; Surgical DOM Swapping</span>
          <span class="about-pill">&#128274; Thread-Safe State Hooks</span>
          <span class="about-pill">&#128230; Zero Runtime Dependencies</span>
        </div>
      </div>
    )";
  }
};

static NavBarComponent s_nav;
static ClockComponent s_clock;
static CounterComponent s_counter;
static RestApiComponent s_api;
static AboutComponent s_about;

static FeatureBlock s_feat_speed("feat-speed", "01", "Native Execution", "Executes business logic directly in C++ RAM with sub-millisecond round-trip response times.");
static FeatureBlock s_feat_spa("feat-spa", "02", "Surgical SPA Routing", "Swaps dynamic DOM elements instantly over lightweight HTTP calls without full-page browser reloads.");
static FeatureBlock s_feat_state("feat-state", "03", "Session State Hooks", "Thread-isolated reactive state per browser session backed by standard synchronization primitives.");
static FeatureBlock s_feat_mono("feat-mono", "04", "Zero Dependencies", "Pure C++ standard library monolith running natively on POSIX and Windows socket stacks.");

static std::string get_api_endpoint() {
  int port = 8080;
  if (const char *env_p = std::getenv("PORT")) {
    try {
      int p = std::stoi(env_p);
      if (p > 0 && p <= 65535) port = p;
    } catch (...) {}
  }
  return "http://127.0.0.1:" + std::to_string(port) + "/api/demo-data";
}

void renderHomePage(np::NovaBuilder &np) {
  np.onLoad([]() {
    g_counter = 0;
    g_quote = "Select a REST API method to test network connectivity.";
  });

  np.onClick("increment", []() { g_counter = g_counter + 1; });
  np.onClick("decrement", []() { g_counter = g_counter - 1; });

  np.onClick("apiGet", []() {
    g_quote = "Fetching GET...";
    std::string res = np::fetch(get_api_endpoint(), "GET");
    g_quote = (res.find("NovaCPP") != std::string::npos) ? ("GET Success!\n" + res) : ("GET Failed: " + res);
  });

  np.onClick("apiPost", []() {
    g_quote = "Sending POST...";
    std::string res = np::fetch(get_api_endpoint(), "POST", R"({"title":"NovaCPP","body":"REST Client","userId":1})", "Content-Type: application/json\r\n");
    g_quote = (res.find("101") != std::string::npos) ? ("POST Success!\n" + res) : ("POST Failed: " + res);
  });

  np.onClick("apiPut", []() {
    g_quote = "Sending PUT...";
    std::string res = np::fetch(get_api_endpoint(), "PUT", R"({"id":1,"title":"Updated","body":"Data","userId":1})", "Content-Type: application/json\r\n");
    g_quote = (res.find("Updated") != std::string::npos) ? "PUT Updated Successfully!" : ("PUT Failed: " + res);
  });

  np.onClick("apiDelete", []() {
    g_quote = "Sending DELETE...";
    std::string res = np::fetch(get_api_endpoint(), "DELETE");
    g_quote = (res.find("{}") != std::string::npos) ? "DELETE Successful!" : ("DELETE Failed: " + res);
  });

  np << R"(<div class="bento-grid">)";
  np.renderComponent(s_nav);
  np.renderComponent(s_clock, 1000);
  np.renderComponent(s_counter);
  np.renderComponent(s_feat_speed);
  np.renderComponent(s_feat_spa);
  np.renderComponent(s_feat_state);
  np.renderComponent(s_feat_mono);
  np.renderComponent(s_api);
  np << R"(</div>)";
}

class DocsComponent : public np::Component {
public:
  DocsComponent() : Component("docs-comp") {}

  void render(np::NovaBuilder &np) override {
    np << R"(
      <div class="bento-card col-12 docs-card">
        <div class="docs-header">
          <div class="badge badge-indigo">DOCUMENTATION</div>
          <h2 class="docs-title">NovaCPP API & Quick Start Guide</h2>
          <p class="docs-desc">
            Explore the zero-dependency C++ web framework APIs. Build reactive SPAs and HTTP microservices with zero external packages.
          </p>
        </div>
        
        <div class="docs-grid">
          <div class="docs-feature-box">
            <div class="docs-feature-head">&#9889; Server &amp; Routes</div>
            <code class="docs-code">app.route("/", renderHome);</code>
            <p class="docs-feature-text">Register declarative page handlers with surgical DOM diffing.</p>
          </div>

          <div class="docs-feature-box">
            <div class="docs-feature-head">&#128274; Reactive State</div>
            <code class="docs-code">np::State&lt;int&gt; count(0);</code>
            <p class="docs-feature-text">Thread-safe session state hooks with microsecond updates.</p>
          </div>

          <div class="docs-feature-box">
            <div class="docs-feature-head">&#127760; Raw Socket Fetch</div>
            <code class="docs-code">np::fetch(url, "GET");</code>
            <p class="docs-feature-text">Zero-dependency TCP socket client with DNS resolution.</p>
          </div>
        </div>

        <div class="docs-footer">
          <a href="https://nova-documentation.vercel.app/" target="_blank" rel="noopener noreferrer" class="docs-cta-btn">
            <span>Open Full Hosted Docs &#8599;</span>
          </a>
          <span class="docs-footer-note">Hosted live on Vercel</span>
        </div>
      </div>
    )";
  }
};

static DocsComponent s_docs;

void renderDocsPage(np::NovaBuilder &np) {
  np << R"(<div class="bento-grid">)";
  np.renderComponent(s_nav);
  np.renderComponent(s_docs);
  np << R"(</div>)";
}

void renderAboutPage(np::NovaBuilder &np) {
  np << R"(<div class="bento-grid">)";
  np.renderComponent(s_nav);
  np.renderComponent(s_about);
  np << R"(</div>)";
}
