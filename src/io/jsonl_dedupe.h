#pragma once

#include <fstream>
#include <string>
#include <unordered_set>

#include "io/minijson.h"

namespace poker_solver::io {

inline std::unordered_set<std::string> LoadExistingSpotIds(const std::string& jsonl_path) {
  std::unordered_set<std::string> ids;

  std::ifstream in(jsonl_path);
  if (!in) {
    return ids;  // treat missing/unreadable as empty
  }

  std::string line;
  while (std::getline(in, line)) {
    if (line.empty()) {
      continue;
    }
    try {
      const auto parsed = minijson::Parse(line);
      if (!parsed.IsObject()) {
        continue;
      }
      const auto& root = parsed.AsObject();
      const auto& meta_v = minijson::RequireObjectKey(root, "metadata");
      if (!meta_v.IsObject()) {
        continue;
      }
      const auto& meta = meta_v.AsObject();
      const auto& id_v = minijson::RequireObjectKey(meta, "spot_id");
      if (!id_v.IsString()) {
        continue;
      }
      ids.insert(id_v.AsString());
    } catch (const std::exception&) {
      // Ignore malformed lines; don't block appends.
      continue;
    }
  }

  return ids;
}

}  // namespace poker_solver::io
