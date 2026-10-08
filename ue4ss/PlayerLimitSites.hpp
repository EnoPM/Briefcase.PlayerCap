#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace briefcase::deceive::detail {

struct ScalarSite {
    std::size_t offset{};
    std::uint8_t width{};
};

struct LimitSites {
    ScalarSite manager_trio, manager_duo, manager_solo;
    ScalarSite session_trio, session_duo, session_solo;
};

// Searches executable code by decoded control flow. Returns offsets within
// text, never build-specific RVAs. Ambiguous or changed code is rejected.
LimitSites find_limit_sites(std::span<const std::uint8_t> text);

} // namespace briefcase::deceive::detail
