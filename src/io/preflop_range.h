#pragma once

#include <algorithm>
#include <cctype>
#include <fstream>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "core/cards.h"
#include "util/rng.h"

namespace poker_solver::io {

struct HoleCards {
  core::Card first;
  core::Card second;
  double weight;  // normalized weight (sums to 1.0 across all combos)
};

struct PreflopRange {
  std::string position;
  std::optional<std::string> villain_position;
  std::string situation;
  std::string stack;
  std::vector<HoleCards> combos;
};

namespace detail {

inline std::string ReadFile(std::string_view path) {
  std::ifstream in{std::string(path)};
  if (!in) {
    throw std::runtime_error("Failed to open preflop range file: " + std::string(path));
  }
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

inline std::optional<std::string> ParseOptionalStringField(const std::string& content,
                                                           std::string_view key) {
  std::regex field_regex("\"" + std::string(key) + "\"\\s*:\\s*(null|\"([^\"]+)\")");
  std::smatch match;
  if (!std::regex_search(content, match, field_regex)) {
    throw std::invalid_argument("Missing required field: " + std::string(key));
  }
  if (match[1] == "null") {
    return std::nullopt;
  }
  return match[2];
}

inline std::optional<std::string> TryParseOptionalStringField(const std::string& content,
                                                              std::string_view key) {
  try {
    return ParseOptionalStringField(content, key);
  } catch (const std::invalid_argument&) {
    return std::nullopt;
  }
}

inline std::string ParseStringField(const std::string& content, std::string_view key) {
  auto value = ParseOptionalStringField(content, key);
  if (!value.has_value()) {
    throw std::invalid_argument("Field cannot be null: " + std::string(key));
  }
  return *value;
}

inline core::Rank RankFromChar(char c) {
  switch (std::toupper(static_cast<unsigned char>(c))) {
    case '2':
      return core::Rank::kTwo;
    case '3':
      return core::Rank::kThree;
    case '4':
      return core::Rank::kFour;
    case '5':
      return core::Rank::kFive;
    case '6':
      return core::Rank::kSix;
    case '7':
      return core::Rank::kSeven;
    case '8':
      return core::Rank::kEight;
    case '9':
      return core::Rank::kNine;
    case 'T':
      return core::Rank::kTen;
    case 'J':
      return core::Rank::kJack;
    case 'Q':
      return core::Rank::kQueen;
    case 'K':
      return core::Rank::kKing;
    case 'A':
      return core::Rank::kAce;
    default:
      throw std::invalid_argument("Invalid rank character in combo label");
  }
}

inline bool SharesCard(const HoleCards& combo, const std::vector<core::Card>& blockers) {
  return std::any_of(blockers.begin(), blockers.end(), [&](const core::Card& blocker) {
    return blocker == combo.first || blocker == combo.second;
  });
}

inline void ExpandPairCombos(core::Rank rank, double weight, std::vector<HoleCards>& out) {
  // Six unique suit combinations for pairs.
  for (int s1 = 0; s1 < 4; ++s1) {
    for (int s2 = s1 + 1; s2 < 4; ++s2) {
      out.push_back(HoleCards{core::Card{rank, static_cast<core::Suit>(s1)},
                              core::Card{rank, static_cast<core::Suit>(s2)}, weight});
    }
  }
}

inline void ExpandSuitedCombos(core::Rank high, core::Rank low, double weight,
                               std::vector<HoleCards>& out) {
  // Four suited combinations.
  for (int s = 0; s < 4; ++s) {
    out.push_back(
        HoleCards{core::Card{high, static_cast<core::Suit>(s)},
                  core::Card{low, static_cast<core::Suit>(s)}, weight});
  }
}

inline void ExpandOffsuitCombos(core::Rank high, core::Rank low, double weight,
                                std::vector<HoleCards>& out) {
  // Twelve off-suit combinations: all suit pairs where suits differ.
  for (int s1 = 0; s1 < 4; ++s1) {
    for (int s2 = 0; s2 < 4; ++s2) {
      if (s1 == s2) {
        continue;
      }
      out.push_back(HoleCards{core::Card{high, static_cast<core::Suit>(s1)},
                              core::Card{low, static_cast<core::Suit>(s2)}, weight});
    }
  }
}

inline std::vector<HoleCards> ParseRangeEntries(const std::string& content) {
  std::vector<HoleCards> combos;
  const std::regex entry_regex(
      "\"([A-Za-z0-9]{2,3})\"\\s*:\\s*\\{[^}]*?\"raise\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)\\s*,\\s*"
      "\"call\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)");

  for (std::sregex_iterator it(content.begin(), content.end(), entry_regex), end; it != end;
       ++it) {
    const auto& match = *it;
    const std::string label = match[1];
    const double raise = std::stod(match[2]);
    const double call = std::stod(match[3]);
    const double weight = raise + call;
    if (weight < 0.0) {
      throw std::invalid_argument("Negative weight in range entry: " + label);
    }
    if (weight == 0.0) {
      continue;
    }
    if (label.size() < 2 || label.size() > 3) {
      throw std::invalid_argument("Invalid combo label: " + label);
    }

    if (label.size() == 2) {
      const core::Rank rank = RankFromChar(label[0]);
      if (label[0] != label[1]) {
        throw std::invalid_argument("Pair label must repeat rank: " + label);
      }
      ExpandPairCombos(rank, weight, combos);
    } else {
      const core::Rank high = RankFromChar(label[0]);
      const core::Rank low = RankFromChar(label[1]);
      const char suited_char = label[2];
      if (suited_char == 's' || suited_char == 'S') {
        ExpandSuitedCombos(high, low, weight, combos);
      } else if (suited_char == 'o' || suited_char == 'O') {
        ExpandOffsuitCombos(high, low, weight, combos);
      } else {
        throw std::invalid_argument("Invalid suitedness in label: " + label);
      }
    }
  }

  if (combos.empty()) {
    throw std::invalid_argument("No range entries parsed from content.");
  }

  double total_weight = 0.0;
  for (const auto& combo : combos) {
    total_weight += combo.weight;
  }
  if (total_weight <= 0.0) {
    throw std::invalid_argument("Total combo weight is zero.");
  }

  for (auto& combo : combos) {
    combo.weight /= total_weight;
  }

  return combos;
}

}  // namespace detail

inline PreflopRange LoadPreflopRange(const std::string& path) {
  const auto content = detail::ReadFile(path);

  PreflopRange range;
  range.position = detail::TryParseOptionalStringField(content, "position").value_or("UNKNOWN");
  range.villain_position = detail::TryParseOptionalStringField(content, "villain_position");
  range.situation = detail::TryParseOptionalStringField(content, "situation").value_or("UNKNOWN");
  range.stack = detail::TryParseOptionalStringField(content, "stack").value_or("UNKNOWN");
  range.combos = detail::ParseRangeEntries(content);
  return range;
}

inline HoleCards SampleHand(const PreflopRange& range, util::Rng& rng,
                            const std::vector<core::Card>& blockers = {}) {
  double total = 0.0;
  for (const auto& combo : range.combos) {
    if (!detail::SharesCard(combo, blockers)) {
      total += combo.weight;
    }
  }
  if (total <= 0.0) {
    throw std::invalid_argument("No available combos after applying blockers.");
  }

  const double draw = rng.UniformReal(0.0, total);
  double accum = 0.0;
  for (const auto& combo : range.combos) {
    if (detail::SharesCard(combo, blockers)) {
      continue;
    }
    accum += combo.weight;
    if (draw <= accum) {
      return combo;
    }
  }

  // Numerical drift fallback.
  for (const auto& combo : range.combos) {
    if (!detail::SharesCard(combo, blockers)) {
      return combo;
    }
  }
  throw std::runtime_error("Sampling failed to find a valid combo.");
}

}  // namespace poker_solver::io
