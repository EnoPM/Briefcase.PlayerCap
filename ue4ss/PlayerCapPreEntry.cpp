#include "PlayerCap.hpp"

#include <Windows.h>
#include <atomic>
#include <cstring>
#include <exception>

namespace {
std::atomic<unsigned> state{}; // 0: not run, 1: applied, 2: failed
char failure[256]{};
}

extern "C" __declspec(dllexport) unsigned __cdecl BriefcasePreEntry() noexcept {
    if (state.load(std::memory_order_acquire) != 0)
        return state.load(std::memory_order_acquire) == 1 ? 0 : 1;
    try {
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
