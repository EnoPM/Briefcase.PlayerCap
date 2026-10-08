#include "PlayerCap.hpp"

#ifdef _WIN32
#include "PlayerLimitSites.hpp"
#include <Windows.h>
#include <array>
#include <cstring>
#include <span>
#include <stdexcept>
#include <vector>

namespace briefcase::deceive {
namespace {
struct ImageSections {
    std::uint8_t *base{};
    std::span<const std::uint8_t> text;
    std::uint32_t text_rva{};
};

ImageSections server_sections() {
    auto *base = reinterpret_cast<std::uint8_t *>(GetModuleHandleW(nullptr));
    if (!base) throw std::runtime_error("Cannot locate the server image");
    const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0 || dos->e_lfanew > 4096)
        throw std::runtime_error("Invalid server image header");
    const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS64 *>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt->FileHeader.NumberOfSections > 32 ||
        nt->OptionalHeader.SizeOfImage < 4096)
        throw std::runtime_error("Invalid server PE layout");
    ImageSections result{base};
    for (unsigned i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        const auto &section = IMAGE_FIRST_SECTION(nt)[i];
        const auto start = section.VirtualAddress;
        const auto size = section.Misc.VirtualSize;
        if (start > nt->OptionalHeader.SizeOfImage ||
            size > nt->OptionalHeader.SizeOfImage - start)
            throw std::runtime_error("Invalid server image section");
        if (std::memcmp(section.Name, ".text", 5) == 0) {
            result.text = {base + start, size};
            result.text_rva = start;
        }
    }
    if (result.text.empty())
        throw std::runtime_error("Server code section is missing");
    return result;
}

struct Change {
    std::uint8_t *address{};
    std::array<std::uint8_t, 8> before{}, after{};
    std::size_t size{};
};

Change scalar_change(std::span<const std::uint8_t> section, detail::ScalarSite site,
                     std::int32_t value) {
    if (site.width != 1 && site.width != 4)
        throw std::runtime_error("Unsupported player-limit immediate width");
    if (site.offset > section.size() || site.width > section.size() - site.offset)
        throw std::runtime_error("Player-limit site is outside executable code");
    Change change{const_cast<std::uint8_t *>(section.data() + site.offset)};
    change.size = site.width;
    std::memcpy(change.before.data(), change.address, change.size);
    std::memcpy(change.after.data(), &value, change.size);
    return change;
}

void write(Change &change, bool restore = false) {
    DWORD previous{};
    if (!VirtualProtect(change.address, change.size, PAGE_EXECUTE_READWRITE, &previous))
        throw std::runtime_error("Cannot make player-limit site writable");
    std::memcpy(change.address, restore ? change.before.data() : change.after.data(), change.size);
    const auto flushed = FlushInstructionCache(GetCurrentProcess(), change.address, change.size) != FALSE;
    DWORD ignored{};
    const auto protected_again = VirtualProtect(change.address, change.size, previous, &ignored) != FALSE;
    if (!flushed || !protected_again)
        throw std::runtime_error("Cannot finalize player-limit change");
}

} // namespace

PlayerCapResult raise_player_ceiling() {
    const auto sections = server_sections();
    const auto sites = detail::find_limit_sites(sections.text);

    std::vector<Change> changes{
        scalar_change(sections.text, sites.manager_trio, 32),
        scalar_change(sections.text, sites.manager_duo, 32),
        scalar_change(sections.text, sites.manager_solo, 32),
        scalar_change(sections.text, sites.session_trio, 32),
        scalar_change(sections.text, sites.session_duo, 32),
        scalar_change(sections.text, sites.session_solo, 32)};

    PlayerCapResult result{
        sections.text_rva + static_cast<std::uint32_t>(sites.manager_trio.offset),
        sections.text_rva + static_cast<std::uint32_t>(sites.session_trio.offset)};
    std::size_t applied{};
    try {
        for (auto &change : changes) {
            ++applied;
            write(change);
        }
    } catch (...) {
        while (applied) {
            try { write(changes[--applied], true); } catch (...) {}
        }
        throw;
    }
    return result;
}

} // namespace briefcase::deceive
#else
#include <stdexcept>
namespace briefcase::deceive {
PlayerCapResult raise_player_ceiling() {
    throw std::runtime_error("The UE4SS PlayerCap patch currently supports Windows servers only");
}
} // namespace briefcase::deceive
#endif
