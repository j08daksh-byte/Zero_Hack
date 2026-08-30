#pragma once
#include <string>

class Database {
public:
  static void connect(const std::string &uri);
  static void query(const std::string &sql);
};
