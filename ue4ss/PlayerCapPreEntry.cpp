#include "PlayerCap.hpp"

#include <Windows.h>
#include <shellapi.h>
#include <atomic>
#include <cstring>
#include <exception>

namespace {
std::atomic<unsigned> state{}; // 0: not run, 1: applied, 2: failed, 3: vanilla UI bypass
char failure[256]{};

bool headless_server() {
    int count{};
    auto **arguments = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!arguments) return false;
    bool unattended{}, null_rhi{};
    for (int index = 1; index < count; ++index) {
        unattended |= _wcsicmp(arguments[index], L"-unattended") == 0;
        null_rhi |= _wcsicmp(arguments[index], L"-nullrhi") == 0;
    }
    LocalFree(arguments);
    return unattended && null_rhi;
}
}

extern "C" __declspec(dllexport) unsigned __cdecl BriefcasePreEntry() noexcept {
    if (state.load(std::memory_order_acquire) != 0)
        return state.load(std::memory_order_acquire) == 2 ? 1 : 0;
    try {
        // The vanilla Server Config editor constructs mode-dependent lists from
        // these limits. Expanding those lists crashes its shutdown path. Keep
        // its unmodified range; headless servers have no editor to tear down.
        if (!headless_server()) {
            state.store(3, std::memory_order_release);
            return 0;
        }
        briefcase::deceive::raise_player_ceiling();
        state.store(1, std::memory_order_release);
        return 0;
    } catch (const std::exception &error) {
        strncpy_s(failure, error.what(), _TRUNCATE);
    } catch (...) {
        strncpy_s(failure, "Unknown PlayerCap pre-entry failure", _TRUNCATE);
    }
    state.store(2, std::memory_order_release);
    return 1;
}

extern "C" __declspec(dllexport) unsigned __cdecl BriefcasePreEntryStatus() noexcept {
    return state.load(std::memory_order_acquire);
}

extern "C" __declspec(dllexport) const char *__cdecl BriefcasePreEntryError() noexcept {
    return failure;
}
