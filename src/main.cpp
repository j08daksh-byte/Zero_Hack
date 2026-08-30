#include <iostream>
#include <string>
#include "../novacpp/html.hpp"
#include "backend/Database.hpp"
#include "backend/Auth.hpp"

void renderHomePage(np::NovaBuilder &np);
void renderDocsPage(np::NovaBuilder &np);
void renderAboutPage(np::NovaBuilder &np);

int main() {
  np::NovaBuilder app;

  Database::connect("memory://zero_dep_db");
  Auth::initialize();

  app.route("/", renderHomePage);
  app.route("/docs", renderDocsPage);
  app.route("/about", renderAboutPage);

  int port = 8080;
  if (const char *env_p = std::getenv("PORT")) {
    try {
      int p = std::stoi(env_p);
      if (p > 0 && p <= 65535) port = p;
    } catch (...) {}
  }

  app.listen(port);
  return 0;
}
