#pragma once

#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace poker_solver::io::minijson {

struct Value;
using Object = std::unordered_map<std::string, Value>;
using Array = std::vector<Value>;

struct Value {
  using Variant = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;
  Variant v;

  bool IsObject() const { return std::holds_alternative<Object>(v); }
  bool IsArray() const { return std::holds_alternative<Array>(v); }
  bool IsString() const { return std::holds_alternative<std::string>(v); }
  bool IsNumber() const { return std::holds_alternative<double>(v); }
  bool IsBool() const { return std::holds_alternative<bool>(v); }
  bool IsNull() const { return std::holds_alternative<std::nullptr_t>(v); }

  const Object& AsObject() const { return std::get<Object>(v); }
  const Array& AsArray() const { return std::get<Array>(v); }
  const std::string& AsString() const { return std::get<std::string>(v); }
  double AsNumber() const { return std::get<double>(v); }
  bool AsBool() const { return std::get<bool>(v); }
};

namespace detail {

inline void SkipWs(std::string_view s, std::size_t& i) {
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
    ++i;
  }
}

inline void Expect(std::string_view s, std::size_t& i, char c) {
  SkipWs(s, i);
  if (i >= s.size() || s[i] != c) {
    throw std::invalid_argument("Invalid JSON: expected character");
  }
  ++i;
}

inline bool Match(std::string_view s, std::size_t& i, std::string_view token) {
  SkipWs(s, i);
  if (s.substr(i, token.size()) == token) {
    i += token.size();
    return true;
  }
  return false;
}

inline std::string ParseString(std::string_view s, std::size_t& i) {
  SkipWs(s, i);
  if (i >= s.size() || s[i] != '"') {
    throw std::invalid_argument("Invalid JSON string");
  }
  ++i;
  std::string out;
  while (i < s.size()) {
    const char c = s[i++];
    if (c == '"') {
      return out;
    }
    if (c == '\\') {
      if (i >= s.size()) {
        throw std::invalid_argument("Invalid JSON escape");
      }
      const char e = s[i++];
      switch (e) {
        case '"':
          out.push_back('"');
          break;
        case '\\':
          out.push_back('\\');
          break;
        case '/':
          out.push_back('/');
          break;
        case 'b':
          out.push_back('\b');
          break;
        case 'f':
          out.push_back('\f');
          break;
        case 'n':
          out.push_back('\n');
          break;
        case 'r':
          out.push_back('\r');
          break;
        case 't':
          out.push_back('\t');
          break;
        case 'u':
          // Minimal parser: accept \uXXXX but do not decode; keep raw.
          // Consume exactly 4 hex digits.
          if (i + 4 > s.size()) {
            throw std::invalid_argument("Invalid JSON unicode escape");
          }
          out.append("\\u");
          out.append(s.substr(i, 4));
          i += 4;
          break;
        default:
          throw std::invalid_argument("Invalid JSON escape sequence");
      }
      continue;
    }
    out.push_back(c);
  }
  throw std::invalid_argument("Unterminated JSON string");
}

inline double ParseNumber(std::string_view s, std::size_t& i) {
  SkipWs(s, i);
  const std::size_t start = i;
  if (i < s.size() && (s[i] == '-' || s[i] == '+')) {
    ++i;
  }
  bool has_digit = false;
  while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
    has_digit = true;
    ++i;
  }
  if (i < s.size() && s[i] == '.') {
    ++i;
    while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
      has_digit = true;
      ++i;
    }
  }
  if (!has_digit) {
    throw std::invalid_argument("Invalid JSON number");
  }
  if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
    ++i;
    if (i < s.size() && (s[i] == '-' || s[i] == '+')) {
      ++i;
    }
    bool exp_digit = false;
    while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
      exp_digit = true;
      ++i;
    }
    if (!exp_digit) {
      throw std::invalid_argument("Invalid JSON exponent");
    }
  }
  const std::string num_str(s.substr(start, i - start));
  return std::stod(num_str);
}

inline Value ParseValue(std::string_view s, std::size_t& i);

inline Array ParseArray(std::string_view s, std::size_t& i) {
  Expect(s, i, '[');
  Array arr;
  SkipWs(s, i);
  if (i < s.size() && s[i] == ']') {
    ++i;
    return arr;
  }
  while (true) {
    arr.push_back(ParseValue(s, i));
    SkipWs(s, i);
    if (i >= s.size()) {
      throw std::invalid_argument("Invalid JSON array");
    }
    if (s[i] == ']') {
      ++i;
      return arr;
    }
    Expect(s, i, ',');
  }
}

inline Object ParseObject(std::string_view s, std::size_t& i) {
  Expect(s, i, '{');
  Object obj;
  SkipWs(s, i);
  if (i < s.size() && s[i] == '}') {
    ++i;
    return obj;
  }
  while (true) {
    const std::string key = ParseString(s, i);
    Expect(s, i, ':');
    Value value = ParseValue(s, i);
    obj.emplace(key, std::move(value));
    SkipWs(s, i);
    if (i >= s.size()) {
      throw std::invalid_argument("Invalid JSON object");
    }
    if (s[i] == '}') {
      ++i;
      return obj;
    }
    Expect(s, i, ',');
  }
}

inline Value ParseValue(std::string_view s, std::size_t& i) {
  SkipWs(s, i);
  if (i >= s.size()) {
    throw std::invalid_argument("Invalid JSON: unexpected end");
  }
  if (s[i] == '"') {
    return Value{ParseString(s, i)};
  }
  if (s[i] == '{') {
    return Value{ParseObject(s, i)};
  }
  if (s[i] == '[') {
    return Value{ParseArray(s, i)};
  }
  if (Match(s, i, "true")) {
    return Value{true};
  }
  if (Match(s, i, "false")) {
    return Value{false};
  }
  if (Match(s, i, "null")) {
    return Value{nullptr};
  }
  return Value{ParseNumber(s, i)};
}

}  // namespace detail

inline Value Parse(std::string_view json) {
  std::size_t i = 0;
  Value v = detail::ParseValue(json, i);
  detail::SkipWs(json, i);
  if (i != json.size()) {
    throw std::invalid_argument("Invalid JSON: trailing characters");
  }
  return v;
}

inline const Value& RequireObjectKey(const Object& obj, std::string_view key) {
  const auto it = obj.find(std::string(key));
  if (it == obj.end()) {
    throw std::invalid_argument("Missing key");
  }
  return it->second;
}

}  // namespace poker_solver::io::minijson

