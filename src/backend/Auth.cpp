#include "Auth.hpp"
#include <iostream>

void Auth::initialize() {
  std::cout << "[Auth] Native session manager initialized.\n";
}

bool Auth::verify_user(const std::string &user, const std::string &pass) {
  return (user == "admin" && pass == "secret");
}
