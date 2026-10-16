#include "MockHost.hpp"
static void playerCap() {
    for (const auto &mode : {"Solo", "Duo", "Trio", "Unknown"}) {
        Backend b({{"soloLimit", 11}, {"duoLimit", 10}});
        b.mode = mode;
        Module mod(L"Briefcase.PlayerCap.dll");
        check(mod.InvalidLoad() == BC_VERSION_MISMATCH, "PlayerCap rejects missing host");
        check(mod.Load(b) == BC_OK, "PlayerCap loads");
#ifdef __linux__
        check(b.staged == std::vector<int>({30, 30, 30, 30}),
              "PlayerCap raises all Linux native ceilings to 30");
        check(b.maxPlayers.empty(), "PlayerCap preserves configured MaxPlayers");
#else
        check(b.staged == std::vector<int>({10, 11, 10, 11}),
              "PlayerCap stages original four patch locations");
        check(b.maxPlayers == (b.mode == "Solo"   ? "11"
                               : b.mode == "Duo"  ? "10"
                               : b.mode == "Trio" ? "12"
                                                  : ""),
              "Mode-specific player cap");
#endif
        check(!b.task && b.clean(), "PlayerCap requires no deferred hook");
        check(mod.Load(b) == BC_INVALID_ARGUMENT, "PlayerCap rejects duplicate Load");
        mod.Stop();
        mod.Stop();
    }
    Backend b({{"soloLimit", 12}, {"duoLimit", 12}});
    Module mod(L"Briefcase.PlayerCap.dll");
    b.supported = false;
    check(mod.Load(b) == BC_VERSION_MISMATCH && b.staged.empty(), "Unsupported build stages nothing");
    b.supported = true;
#ifndef __linux__
    b.config["soloLimit"] = 13;
    check(mod.Load(b) == BC_INVALID_ARGUMENT && b.staged.empty(), "Invalid PlayerCap config stages nothing");
    b.config["soloLimit"] = 12;
#endif
    check(mod.Load(b) == BC_OK, "Failed PlayerCap load can be retried");
    mod.Stop();
}
int main() { try { playerCap(); std::cout << "PASS " << checks << " lifecycle checks\n"; return 0; } catch(const std::exception& e) { std::cerr << e.what() << "\n"; return 1; } }
