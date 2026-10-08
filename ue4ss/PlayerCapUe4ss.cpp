#include <Common.hpp>
#include "ModVersion.hpp"

#include <Briefcase/DeceiveInc/Paths.hpp>
#include <DynamicOutput/Output.hpp>
#include <Helpers/String.hpp>
#include <Mod/CppUserModBase.hpp>
#include <Windows.h>

namespace {
using namespace RC;

bool early_patch_applied() {
    const auto path = briefcase::deceive::module_directory(
        reinterpret_cast<const void *>(&early_patch_applied)) / L"BriefcasePreEntry.dll";
    const auto module = GetModuleHandleW(path.c_str());
    if (!module)
        throw std::runtime_error("PlayerCap pre-entry helper was not loaded by the server proxy");
    const auto status = reinterpret_cast<unsigned(__cdecl *)()>(
        GetProcAddress(module, "BriefcasePreEntryStatus"));
    if (!status) throw std::runtime_error("PlayerCap pre-entry status is unavailable");
    const auto value = status();
    if (value != 1 && value != 3)
        throw std::runtime_error("PlayerCap native ceiling was not raised before game entry");
    return value == 1;
}

class PlayerCapUe4ss final : public CppUserModBase {
  public:
    PlayerCapUe4ss() {
        ModName = STR("Briefcase.PlayerCap");
        ModVersion = briefcase_mod_version;
        ModAuthors = STR("EnoPM");
        if (early_patch_applied()) {
            ModDescription = STR("Raises Solo, Duo and Trio ceilings to 32 players");
            Output::send(STR("[Briefcase.PlayerCap] headless server: Solo/Duo/Trio ceilings=32 verified; configured MaxPlayers remains effective\n"));
        } else {
            ModDescription = STR("Inactive while the vanilla Server Config window is available");
            Output::send(STR("[Briefcase.PlayerCap] vanilla Server Config mode: native limits unchanged to avoid its shutdown crash\n"));
        }
    }
};
} // namespace

extern "C" __declspec(dllexport) RC::CppUserModBase *start_mod() {
    try {
        return new PlayerCapUe4ss();
    } catch (const std::exception &error) {
        RC::Output::send<RC::LogLevel::Warning>(STR("[Briefcase.PlayerCap] load failed: {}\n"),
                                                RC::to_wstring(error.what()));
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

extern "C" __declspec(dllexport) void uninstall_mod(RC::CppUserModBase *mod) { delete mod; }
