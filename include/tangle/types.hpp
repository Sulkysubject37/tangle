#pragma once
#include <cstdint>
#include <string>
#include <optional>

namespace tangle {

using NodeId = std::uint32_t;
using EdgeId = std::uint64_t;

using ProteinId = std::string;
using GeneSymbol = std::string;

using Weight = double;

} // namespace tangle
