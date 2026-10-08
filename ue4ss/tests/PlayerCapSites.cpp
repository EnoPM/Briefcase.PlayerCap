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

std::uint32_t read32(const std::vector<std::uint8_t> &data, std::size_t at) {
    std::uint32_t result{};
    check(at <= data.size() && data.size() - at >= sizeof(result), "Invalid PE offset");
    std::memcpy(&result, data.data() + at, sizeof(result));
    return result;
}

void installed_server(const std::filesystem::path &path, bool print_sites = false) {
    std::ifstream file(path, std::ios::binary);
    check(bool(file), "Cannot open local server executable");
    const std::vector<std::uint8_t> image{std::istreambuf_iterator<char>(file), {}};
    const auto pe = read32(image, 0x3c);
    const auto count = image.at(pe + 6);
    const auto optional_size = image.at(pe + 20);
    const auto section_headers = pe + 24 + optional_size;
    bool text_found = false;
    for (unsigned i = 0; i < count; ++i) {
        const auto header = section_headers + i * 40;
        const auto raw_size = read32(image, header + 16);
        const auto raw_offset = read32(image, header + 20);
        const auto rva = read32(image, header + 12);
        check(raw_offset <= image.size() && raw_size <= image.size() - raw_offset,
              "Invalid PE section file span");
        if (std::memcmp(image.data() + header, ".text", 5) == 0) {
            const auto sites = find_limit_sites({image.data() + raw_offset, raw_size});
            if (print_sites) {
                const auto show = [&](const char *name, briefcase::deceive::detail::ScalarSite site) {
                    std::cout << name << " RVA=0x" << std::hex << rva + site.offset << std::dec
                              << " width=" << unsigned(site.width) << '\n';
                };
                show("manager trio", sites.manager_trio);
                show("manager duo", sites.manager_duo);
                show("manager solo", sites.manager_solo);
                show("session trio", sites.session_trio);
                show("session duo", sites.session_duo);
                show("session solo", sites.session_solo);
            }
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
            text_found = true;
        }
    }
    check(text_found, "Server player-limit code is missing");
}
} // namespace

int main(int argc, char **argv) {
    try {
        synthetic_code();
        if (argc > 1) installed_server(argv[1], argc > 2 && std::strcmp(argv[2], "--sites") == 0);
        std::cout << "PASS PlayerCap native site discovery and ambiguity contracts\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
