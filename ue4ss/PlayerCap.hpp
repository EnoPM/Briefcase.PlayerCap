#pragma once

#include <cstdint>

namespace briefcase::deceive {

struct PlayerCapResult {
    std::uint32_t manager_rva{};
    std::uint32_t session_rva{};
};

// Raises native Solo, Duo and Trio ceilings to 32 before game entry.
// The effective limit remains the vanilla MaxPlayers INI/editor setting.
// Ambiguous code or unexpected editor metadata fails closed.
PlayerCapResult raise_player_ceiling();

} // namespace briefcase::deceive
