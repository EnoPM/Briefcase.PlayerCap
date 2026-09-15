#include "../PlayerCap.hpp"
#include "ConfigDefaults.hpp"
#include <iostream>
static int checks;
void check(bool b) {
    ++checks;
    if (!b)
        throw std::runtime_error("Player Cap contract #" + std::to_string(checks));
}
template <class F> void rejects(F f) {
    bool b = false;
    try {
        f();
    } catch (...) {
        b = true;
    }
    check(b);
}
int main() {
    try {
        auto schema = nlohmann::json::parse(player_cap::schema);
        auto defaults = config_defaults(schema);
        auto limits = player_cap::Limits::parse(defaults.dump());
        check(limits.solo == 12 && limits.duo == 12);
        for (int solo = 1; solo <= 12; ++solo)
            for (int duo = 1; duo <= 12; ++duo) {
                auto data = nlohmann::json{{"soloLimit", solo}, {"duoLimit", duo}};
                auto parsed = player_cap::Limits::parse(data.dump());
                check(parsed.active("Solo") == solo && parsed.active("Duo") == duo);
                check(parsed.active("Trio") == 12 && !parsed.active("Unknown"));
                auto p = player_cap::patches(parsed);
                check(p[0].replacement == duo && p[1].replacement == solo && p[2].replacement == duo &&
                      p[3].replacement == solo);
            }
        for (auto value :
             {nlohmann::json(0), nlohmann::json(13), nlohmann::json(3.5), nlohmann::json("12")}) {
            for (auto key : {"soloLimit", "duoLimit"}) {
                auto bad = defaults;
                bad[key] = value;

                rejects([&] { player_cap::Limits::parse(bad.dump()); });
            }
        }
        check(player_cap::manager[12] == 12 && player_cap::session[17] == 12);
        std::cout << "PASS " << checks << " Player Cap contract checks\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
