#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

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

// Additional mode-dependent limits used while constructing and editing the
// dedicated server settings UI. Exactly four validated calculations are expected.
std::vector<LimitSites> find_editor_limit_sites(std::span<const std::uint8_t> text);

// Finds the native server editor's MaxPlayers numeric descriptor by its
// relocated property/label pointers and checked numeric range.
std::size_t find_ui_maximum(std::span<const std::uint8_t> rdata);

} // namespace briefcase::deceive::detail
