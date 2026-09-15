#pragma once
#include <Briefcase/StartupApi.h>
#include <array>
#include <nlohmann/json.hpp>
#include <optional>
#include <string_view>
namespace player_cap {
inline constexpr const char *game_hash = "78afe1dbeecb09027c274def4f0ac855b447dc52ffe3cd9482c1be4341b0dae6";
inline constexpr std::string_view schema = R"({
  "type":"object",
  "properties":{
    "soloLimit":{"type":"integer","minimum":1,"maximum":12,"default":12,
      "description":"Maximum Solo players. Restart required."},
    "duoLimit":{"type":"integer","minimum":1,"maximum":12,"default":12,
      "description":"Maximum Duo players (individual players, not teams). Restart required."}
  }
})";
struct Limits {
    int32_t solo = 12, duo = 12;
    static Limits parse(std::string_view text) {
        const auto j = nlohmann::json::parse(text);
        auto read = [&](const char *key) {
            const auto &v = j.at(key);
            if (!v.is_number_integer() || v < 1 || v > 12)
                throw std::runtime_error("Limit must be integer 1..12");
            return v.get<int32_t>();
        };
        return {read("soloLimit"), read("duoLimit")};
    }
    std::optional<int32_t> active(std::string_view mode) const {
        if (mode == "Solo")
            return solo;
        if (mode == "Duo")
            return duo;
        if (mode == "Trio")
            return 12;       // Vanilla ceiling, independent from Solo/Duo configuration.
        return std::nullopt; // Unknown modes retain their settings.
    }
};
inline constexpr uint8_t manager[] = {0x83, 0xE9, 0x02, 0x74, 0x15, 0x83, 0xF9, 0x01, 0x74, 0x08, 0x41, 0xB9,
                                      0x0C, 0,    0,    0,    0xEB, 0x0E, 0x41, 0xB9, 0x0A, 0,    0,    0,
                                      0xEB, 0x06, 0x41, 0xB9, 0x08, 0,    0,    0,    0xC6, 0x47, 0x51, 0};
inline constexpr uint8_t session[] = {0x41, 0x0F, 0xB6, 0x48, 0x60, 0x83, 0xE9, 0x02, 0x74, 0x11,
                                      0x83, 0xF9, 0x01, 0x74, 0x05, 0x8D, 0x42, 0x0C, 0xEB, 0x0C,
                                      0xB8, 0x0A, 0,    0,    0,    0xEB, 0x05, 0xB8, 0x08, 0,
                                      0,    0,    0x41, 0x8B, 0x48, 0x48, 0x83, 0xF9, 0x01};
inline std::array<BcImmediatePatch, 4> patches(Limits limits) {
    return {{{sizeof(BcImmediatePatch), 0x10dc858, manager, sizeof(manager), 20, limits.duo, 0},
             {sizeof(BcImmediatePatch), 0x10dc858, manager, sizeof(manager), 28, limits.solo, 0},
             {sizeof(BcImmediatePatch), 0x1362da6, session, sizeof(session), 21, limits.duo, 0},
             {sizeof(BcImmediatePatch), 0x1362da6, session, sizeof(session), 28, limits.solo, 0}}};
}
} // namespace player_cap
