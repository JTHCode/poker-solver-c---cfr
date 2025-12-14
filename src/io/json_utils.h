#pragma once

#include <string>
#include <string_view>

namespace poker_solver::io::json {

inline std::string EscapeString(std::string_view input) {
  std::string out;
  out.reserve(input.size() + 8);
  for (const char c : input) {
    switch (c) {
      case '\"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\b':
        out += "\\b";
        break;
      case '\f':
        out += "\\f";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out.push_back(c);
        break;
    }
  }
  return out;
}

inline std::string Quote(std::string_view input) { return "\"" + EscapeString(input) + "\""; }

}  // namespace poker_solver::io::json
