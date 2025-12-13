#pragma once

#include <array>

#include "core/cards.h"

namespace poker_solver::core {

struct Board {
  std::array<Card, 3> flop{};
  Card turn{};
  Card river{};
};

}  // namespace poker_solver::core
