#pragma once

#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace poker_solver::io {

class JsonlWriter {
 public:
  explicit JsonlWriter(const std::string& path) : out_(path, std::ios::app) {
    if (!out_) {
      throw std::runtime_error("Failed to open JSONL output file: " + path);
    }
  }

  void AppendLine(std::string_view line) {
    if (line.empty()) {
      throw std::invalid_argument("JSONL line cannot be empty");
    }
    if (line.find('\n') != std::string_view::npos || line.find('\r') != std::string_view::npos) {
      throw std::invalid_argument("JSONL line must be single-line (no newlines)");
    }
    out_ << line << "\n";
    if (!out_) {
      throw std::runtime_error("Failed to write JSONL line");
    }
  }

  void Flush() { out_.flush(); }

 private:
  std::ofstream out_;
};

}  // namespace poker_solver::io
