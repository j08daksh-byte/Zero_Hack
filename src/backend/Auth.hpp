#pragma once
#include <string>

class Auth {
public:
  static void initialize();
  static bool verify_user(const std::string &user, const std::string &pass);
};
