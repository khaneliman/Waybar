#include "include/util/string.hpp"

#include <algorithm>
#include <iostream>
#include <string>

std::string ltrim(const std::string& s) {
  size_t begin = s.find_first_not_of(WHITESPACE);
  return (begin == std::string::npos) ? "" : s.substr(begin);
}

std::string rtrim(const std::string& s) {
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(const std::string& s) { return rtrim(ltrim(s)); }

std::string capitalize(const std::string& str) {
  std::string result = str;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char c) { return std::toupper(c); });
  return result;
}
