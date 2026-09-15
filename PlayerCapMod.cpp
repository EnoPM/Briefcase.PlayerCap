#include "PlayerCapMod.hpp"
#include <cstring>
#include <format>

namespace player_cap {
void PlayerCapMod::Log(std::string_view message) const noexcept {
    if (api_ && api_->log)
        briefcase::Api(api_).log(message);
}
bool PlayerCapMod::IsSupportedBuild() const {
    BcBuild build{sizeof(build)};
    return api_->get_build && api_->get_build(api_->context, &build) == BC_OK &&
           build.pe_timestamp == 0x6a966107 && build.image_size == 0x05b60000 &&
           std::strcmp(build.executable_sha256, game_hash) == 0;
}
void PlayerCapMod::StageChanges() const {
    const briefcase::Services services(api_);
    constexpr auto section = "/Script/DeceiveInc.TripwireServerSettings";
    const auto mode = services.ini("server-settings", section, "GameMode");
    services.stage(game_hash, patches(limits_));
    if (auto cap = limits_.active(mode)) {
        const auto value = std::to_string(*cap);
        services.stage_ini("server-settings", section, "MaxPlayers", value.c_str());
        Log(std::format("Staged MaxPlayers={} for {}; Solo={}, Duo={}; Trio unchanged", *cap, mode,
                        limits_.solo, limits_.duo));
    } else {
        Log(std::format("Mode {}: MaxPlayers unchanged; staged Solo={}, Duo={}; Trio unchanged", mode,
                        limits_.solo, limits_.duo));
    }
    Log("Four validated immediates staged (dedicated manager + EOS session); host commit pending");
}
BcResult PlayerCapMod::Load(const BcApi *api) noexcept {
    if (api_)
        return BC_INVALID_ARGUMENT;
    if (!api || api->size < sizeof(BcApi) || api->version != BC_API_VERSION)
        return BC_VERSION_MISMATCH;
    api_ = api;
    try {
        if (!IsSupportedBuild()) {
            Log("Unsupported game build; no changes staged");
            Unload();
            return BC_VERSION_MISMATCH;
        }
        limits_ = Limits::parse(briefcase::Services(api_).config(schema));
        StageChanges();
        return BC_OK;
    } catch (const std::exception &e) {
        try {
            Log(std::string("Player Cap rejected: ") + e.what());
        } catch (...) {
        }
        Unload();
        return BC_INVALID_ARGUMENT;
    } catch (...) {
        Log("Player Cap rejected: unexpected exception");
        Unload();
        return BC_INTERNAL;
    }
}
void PlayerCapMod::Unload() noexcept {
    // Process-local patches belong to the host transaction and disappear at process exit.
    // Never restore instructions during a live session.
    api_ = nullptr;
    limits_ = {};
}
} // namespace player_cap
