#include "../PlayerLimitSites.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

using briefcase::deceive::detail::find_limit_sites;
using briefcase::deceive::detail::find_editor_limit_sites;
using briefcase::deceive::detail::find_ui_maximum;

namespace {
void check(bool good, const char *message) {
    if (!good) throw std::runtime_error(message);
}

template <class F> void rejects(F action, const char *message) {
    try { action(); } catch (const std::runtime_error &) { return; }
    throw std::runtime_error(message);
}

template <std::size_t N> void place(std::vector<std::uint8_t> &out, std::size_t at,
                                    const std::array<std::uint8_t, N> &bytes) {
    std::copy(bytes.begin(), bytes.end(), out.begin() + at);
}

void synthetic_code() {
    // Captured current-server instructions are test data; the resolver itself
    // decodes control flow and carries no patch RVAs or full byte signatures.
    constexpr std::array<std::uint8_t, 36> manager{
        0x83,0xe9,2,0x74,0x15,0x83,0xf9,1,0x74,8,0x41,0xb9,12,0,0,0,
        0xeb,0x0e,0x41,0xb9,10,0,0,0,0xeb,6,0x41,0xb9,8,0,0,0,0xc6,0x47,0x51,0};
    constexpr std::array<std::uint8_t, 39> session{
        0x41,0x0f,0xb6,0x48,0x60,0x83,0xe9,2,0x74,0x11,0x83,0xf9,1,
        0x74,5,0x8d,0x42,12,0xeb,12,0xb8,10,0,0,0,0xeb,5,0xb8,8,0,0,0,
        0x41,0x8b,0x48,0x48,0x83,0xf9,1};
    std::vector<std::uint8_t> code(256, 0xcc);
    place(code, 17, manager);
    place(code, 111, session);
    const auto found = find_limit_sites(code);
    check(found.manager_trio.offset == 29 && found.manager_trio.width == 4 &&
          found.manager_duo.offset == 37 && found.manager_solo.offset == 45,
          "Manager immediates were not decoded correctly");
    check(found.session_trio.offset == 128 && found.session_trio.width == 1 &&
          found.session_duo.offset == 132 && found.session_solo.offset == 139,
          "EOS session immediates were not decoded correctly");
    code[17 + 4] = 0x16;
    rejects([&] { find_limit_sites(code); }, "Changed branch target was accepted");
    code[17 + 4] = 0x15;
    place(code, 170, manager);
    rejects([&] { find_limit_sites(code); }, "Two manager sites were accepted");
}

void synthetic_editor() {
    std::vector<std::uint8_t> data(320);
    auto put_wide = [&](std::size_t at, const wchar_t *value) {
        for (std::size_t i = 0; value[i]; ++i) data[at + 2 * i] =
            static_cast<std::uint8_t>(value[i]);
    };
    put_wide(16, L"MaxPlayers");
    put_wide(80, L"Max Players");
    const auto key = reinterpret_cast<std::uintptr_t>(data.data() + 16);
    const auto label = reinterpret_cast<std::uintptr_t>(data.data() + 80);
    constexpr double minimum = 1.0, maximum = 32.0;
    std::memcpy(data.data() + 192, &key, sizeof(key));
    std::memcpy(data.data() + 200, &label, sizeof(label));
    std::memcpy(data.data() + 224, &minimum, sizeof(minimum));
    std::memcpy(data.data() + 232, &maximum, sizeof(maximum));
    check(find_ui_maximum(data) == 232, "Vanilla editor maximum was not found");
    constexpr double wrong_minimum = 0.0;
    std::memcpy(data.data() + 224, &wrong_minimum, sizeof(wrong_minimum));
    rejects([&] { find_ui_maximum(data); }, "Invalid editor range was accepted");
}

void synthetic_editor_code() {
    constexpr std::array<std::uint8_t, 32> register_limit{
        0x83,0xe9,2,0x74,0x13,0x83,0xf9,1,0x74,7,0xb8,12,0,0,0,0xeb,12,
        0xb8,10,0,0,0,0xeb,5,0xb8,8,0,0,0,0x8b,0x4b,0x48};
    constexpr std::array<std::uint8_t, 35> array_limit{
        0x83,0xe9,2,0x74,0x13,0x83,0xf9,1,0x74,7,0xbb,12,0,0,0,0xeb,12,
        0xbb,10,0,0,0,0xeb,5,0xbb,8,0,0,0,0xf2,0x41,0x0f,0x2c,0x45,0x28};
    constexpr std::array<std::uint8_t, 42> stack_limit{
        0x83,0xe9,1,0x74,0x17,0x83,0xf9,1,0x74,9,
        0xc7,0x45,0x88,12,0,0,0,0xeb,0x10,
        0xc7,0x45,0x88,10,0,0,0,0xeb,7,
        0xc7,0x45,0x88,8,0,0,0,0x0f,0x28,0x05,0x38,0xbf,0xe3,0x02};
    std::vector<std::uint8_t> code(400, 0xcc);
    place(code, 20, register_limit);
    place(code, 100, array_limit);
    place(code, 180, register_limit);
    place(code, 260, stack_limit);
    const auto found = find_editor_limit_sites(code);
    check(found.size() == 4, "Editor mode calculations were not decoded");
    for (const auto &site : found)
        check(code.at(site.manager_trio.offset) == 12 &&
              code.at(site.manager_duo.offset) == 10 &&
              code.at(site.manager_solo.offset) == 8,
              "Editor mode immediates were not decoded correctly");
    code[20 + 4] = 0x14;
    rejects([&] { find_editor_limit_sites(code); }, "Changed editor branch was accepted");
}

std::uint32_t read32(const std::vector<std::uint8_t> &data, std::size_t at) {
    std::uint32_t result{};
    check(at <= data.size() && data.size() - at >= sizeof(result), "Invalid PE offset");
    std::memcpy(&result, data.data() + at, sizeof(result));
    return result;
}

std::uint64_t read64(const std::vector<std::uint8_t> &data, std::size_t at) {
    std::uint64_t result{};
    check(at <= data.size() && data.size() - at >= sizeof(result), "Invalid PE offset");
    std::memcpy(&result, data.data() + at, sizeof(result));
    return result;
}

void installed_server(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    check(bool(file), "Cannot open local server executable");
    const std::vector<std::uint8_t> image{std::istreambuf_iterator<char>(file), {}};
    const auto pe = read32(image, 0x3c);
    const auto count = image.at(pe + 6);
    const auto optional_size = image.at(pe + 20);
    const auto preferred_base = read64(image, pe + 24 + 24);
    const auto section_headers = pe + 24 + optional_size;
    bool text_found = false, editor_found = false;
    for (unsigned i = 0; i < count; ++i) {
        const auto header = section_headers + i * 40;
        const auto raw_size = read32(image, header + 16);
        const auto raw_offset = read32(image, header + 20);
        const auto rva = read32(image, header + 12);
        check(raw_offset <= image.size() && raw_size <= image.size() - raw_offset,
              "Invalid PE section file span");
        if (std::memcmp(image.data() + header, ".text", 5) == 0) {
            const auto sites = find_limit_sites({image.data() + raw_offset, raw_size});
            const auto editor = find_editor_limit_sites({image.data() + raw_offset, raw_size});
            auto immediate_at = [&](briefcase::deceive::detail::ScalarSite site) {
                check(site.offset <= raw_size && site.width <= raw_size - site.offset,
                      "Resolved player-limit site is outside executable code");
                std::uint32_t value{};
                std::memcpy(&value, image.data() + raw_offset + site.offset, site.width);
                return value;
            };
            check(immediate_at(sites.manager_trio) == 12 &&
                  immediate_at(sites.manager_duo) == 10 &&
                  immediate_at(sites.manager_solo) == 8 &&
                  immediate_at(sites.session_trio) == 12 &&
                  immediate_at(sites.session_duo) == 10 &&
                  immediate_at(sites.session_solo) == 8,
                  "Installed server resolved unexpected vanilla player limits");
            check(editor.size() == 4, "Installed server editor calculations are missing");
            for (const auto &site : editor)
                check(immediate_at(site.manager_trio) == 12 &&
                      immediate_at(site.manager_duo) == 10 &&
                      immediate_at(site.manager_solo) == 8,
                      "Installed server editor has unexpected vanilla player limits");
            text_found = true;
        }
        if (std::memcmp(image.data() + header, ".rdata", 6) == 0) {
            std::vector<std::uint8_t> rdata(image.begin() + raw_offset,
                                            image.begin() + raw_offset + raw_size);
            // The Windows loader rebases absolute pointers. Recreate only the
            // relocations into this section for a read-only offline fixture.
            for (std::size_t at = 0; at + 8 <= rdata.size(); at += 8) {
                std::uint64_t value{};
                std::memcpy(&value, rdata.data() + at, sizeof(value));
                if (value < preferred_base + rva ||
                    value >= preferred_base + rva + raw_size) continue;
                const auto relocated = reinterpret_cast<std::uintptr_t>(
                    rdata.data() + value - preferred_base - rva);
                std::memcpy(rdata.data() + at, &relocated, sizeof(relocated));
            }
            const auto maximum = find_ui_maximum(rdata);
            double current{};
            std::memcpy(&current, rdata.data() + maximum, sizeof(current));
            check(current == 32.0, "Unexpected vanilla editor maximum");
            editor_found = true;
        }
    }
    check(text_found && editor_found, "Server code or editor metadata is missing");
}
} // namespace

int main(int argc, char **argv) {
    try {
        synthetic_code();
        synthetic_editor();
        synthetic_editor_code();
        if (argc > 1) installed_server(argv[1]);
        std::cout << "PASS PlayerCap native site discovery and ambiguity contracts\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
