#include "Database.hpp"
#include <iostream>

void Database::connect(const std::string &uri) {
  std::cout << "[Database] In-memory store connected: " << uri << "\n";
}

void Database::query(const std::string &sql) {
  std::cout << "[Database] Executing: " << sql << "\n";
}
