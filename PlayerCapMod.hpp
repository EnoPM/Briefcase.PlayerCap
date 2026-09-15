#pragma once
#include "PlayerCap.hpp"
#include <Briefcase/Startup.hpp>

namespace player_cap {
// Startup-only mod. The host commits staged patches; this object owns no live game hook.
class PlayerCapMod {
    const BcApi *api_{};
    Limits limits_;

    bool IsSupportedBuild() const;
    void StageChanges() const;
    void Log(std::string_view message) const noexcept;

  public:
    PlayerCapMod() = default;
    PlayerCapMod(const PlayerCapMod &) = delete;
    PlayerCapMod &operator=(const PlayerCapMod &) = delete;
    BcResult Load(const BcApi *api) noexcept;
    void Unload() noexcept;
};
} // namespace player_cap
