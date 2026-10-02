#include "PlayerLimitSites.hpp"

#include <Zydis/Zydis.h>
#include <array>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace briefcase::deceive::detail {
namespace {
struct Instruction {
    ZydisDecodedInstruction code{};
    std::array<ZydisDecodedOperand, ZYDIS_MAX_OPERAND_COUNT> operands{};
    std::size_t at{};
};

bool decode(const ZydisDecoder &decoder, std::span<const std::uint8_t> text,
            std::size_t &cursor, Instruction &out) {
    if (cursor >= text.size()) return false;
    out = {};
    out.at = cursor;
    if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, text.data() + cursor,
                                               text.size() - cursor, &out.code,
                                               out.operands.data()))) return false;
    cursor += out.code.length;
    return true;
}

bool reg_imm(const Instruction &ins, ZydisMnemonic mnemonic, ZydisRegister reg,
             std::uint64_t value) {
    return ins.code.mnemonic == mnemonic && ins.code.operand_count_visible == 2 &&
           ins.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
           ins.operands[0].reg.value == reg &&
           ins.operands[1].type == ZYDIS_OPERAND_TYPE_IMMEDIATE &&
           ins.operands[1].imm.value.u == value;
}

std::optional<std::size_t> branch_target(const Instruction &ins, ZydisMnemonic mnemonic) {
    if (ins.code.mnemonic != mnemonic || ins.code.operand_count_visible != 1 ||
        ins.operands[0].type != ZYDIS_OPERAND_TYPE_IMMEDIATE ||
        !ins.operands[0].imm.is_relative) return std::nullopt;
    const auto target = static_cast<std::int64_t>(ins.at) + ins.code.length +
                        ins.operands[0].imm.value.s;
    if (target < 0) return std::nullopt;
    return static_cast<std::size_t>(target);
}

std::optional<ScalarSite> immediate(const Instruction &ins, std::uint8_t bits) {
    if (ins.code.raw.imm[0].size != bits) return std::nullopt;
    return ScalarSite{ins.at + ins.code.raw.imm[0].offset,
                      static_cast<std::uint8_t>(bits / 8)};
}

std::optional<LimitSites> manager_at(const ZydisDecoder &decoder,
                                     std::span<const std::uint8_t> text, std::size_t at) {
    std::size_t cursor = at;
    std::array<Instruction, 10> ins{};
    for (auto &item : ins) if (!decode(decoder, text, cursor, item)) return std::nullopt;
    if (!reg_imm(ins[0], ZYDIS_MNEMONIC_SUB, ZYDIS_REGISTER_ECX, 2) ||
        !reg_imm(ins[2], ZYDIS_MNEMONIC_CMP, ZYDIS_REGISTER_ECX, 1) ||
        !reg_imm(ins[4], ZYDIS_MNEMONIC_MOV, ZYDIS_REGISTER_R9D, 12) ||
        !reg_imm(ins[6], ZYDIS_MNEMONIC_MOV, ZYDIS_REGISTER_R9D, 10) ||
        !reg_imm(ins[8], ZYDIS_MNEMONIC_MOV, ZYDIS_REGISTER_R9D, 8) ||
        branch_target(ins[1], ZYDIS_MNEMONIC_JZ) != ins[8].at ||
        branch_target(ins[3], ZYDIS_MNEMONIC_JZ) != ins[6].at ||
        branch_target(ins[5], ZYDIS_MNEMONIC_JMP) != ins[9].at ||
        branch_target(ins[7], ZYDIS_MNEMONIC_JMP) != ins[9].at) return std::nullopt;
    const auto trio = immediate(ins[4], 32), duo = immediate(ins[6], 32),
               solo = immediate(ins[8], 32);
    if (!trio || !duo || !solo) return std::nullopt;
    return LimitSites{*trio, *duo, *solo};
}

std::optional<LimitSites> session_at(const ZydisDecoder &decoder,
                                     std::span<const std::uint8_t> text, std::size_t at) {
    std::size_t cursor = at;
    std::array<Instruction, 11> ins{};
    for (auto &item : ins) if (!decode(decoder, text, cursor, item)) return std::nullopt;
    const auto &first = ins[0];
    if (first.code.mnemonic != ZYDIS_MNEMONIC_MOVZX ||
        first.code.operand_count_visible != 2 ||
        first.operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
        first.operands[0].reg.value != ZYDIS_REGISTER_ECX ||
        first.operands[1].type != ZYDIS_OPERAND_TYPE_MEMORY ||
        first.operands[1].mem.base != ZYDIS_REGISTER_R8 ||
        !reg_imm(ins[1], ZYDIS_MNEMONIC_SUB, ZYDIS_REGISTER_ECX, 2) ||
        !reg_imm(ins[3], ZYDIS_MNEMONIC_CMP, ZYDIS_REGISTER_ECX, 1) ||
        ins[5].code.mnemonic != ZYDIS_MNEMONIC_LEA ||
        ins[5].code.operand_count_visible != 2 ||
        ins[5].operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
        ins[5].operands[0].reg.value != ZYDIS_REGISTER_EAX ||
        ins[5].operands[1].type != ZYDIS_OPERAND_TYPE_MEMORY ||
        ins[5].operands[1].mem.base != ZYDIS_REGISTER_RDX ||
        ins[5].operands[1].mem.disp.value != 12 ||
        !reg_imm(ins[7], ZYDIS_MNEMONIC_MOV, ZYDIS_REGISTER_EAX, 10) ||
        !reg_imm(ins[9], ZYDIS_MNEMONIC_MOV, ZYDIS_REGISTER_EAX, 8) ||
        branch_target(ins[2], ZYDIS_MNEMONIC_JZ) != ins[9].at ||
        branch_target(ins[4], ZYDIS_MNEMONIC_JZ) != ins[7].at ||
        branch_target(ins[6], ZYDIS_MNEMONIC_JMP) != ins[10].at ||
        branch_target(ins[8], ZYDIS_MNEMONIC_JMP) != ins[10].at ||
        ins[10].code.mnemonic != ZYDIS_MNEMONIC_MOV ||
        ins[10].operands[0].type != ZYDIS_OPERAND_TYPE_REGISTER ||
        ins[10].operands[0].reg.value != ZYDIS_REGISTER_ECX ||
        ins[10].operands[1].type != ZYDIS_OPERAND_TYPE_MEMORY ||
        ins[10].operands[1].mem.base != ZYDIS_REGISTER_R8 ||
        ins[5].code.raw.disp.size != 8) return std::nullopt;
    const auto duo = immediate(ins[7], 32), solo = immediate(ins[9], 32);
    if (!duo || !solo) return std::nullopt;
    return LimitSites{{}, {}, {},
                      {ins[5].at + ins[5].code.raw.disp.offset, 1}, *duo, *solo};
}

std::optional<LimitSites> editor_at(const ZydisDecoder &decoder,
                                    std::span<const std::uint8_t> text, std::size_t at,
                                    bool stack_form) {
    std::size_t cursor = at;
    std::array<Instruction, 10> ins{};
    for (auto &item : ins) if (!decode(decoder, text, cursor, item)) return std::nullopt;
    if (!reg_imm(ins[0], ZYDIS_MNEMONIC_SUB, ZYDIS_REGISTER_ECX, stack_form ? 1 : 2) ||
        !reg_imm(ins[2], ZYDIS_MNEMONIC_CMP, ZYDIS_REGISTER_ECX, 1) ||
        branch_target(ins[1], ZYDIS_MNEMONIC_JZ) != ins[8].at ||
        branch_target(ins[3], ZYDIS_MNEMONIC_JZ) != ins[6].at ||
        branch_target(ins[5], ZYDIS_MNEMONIC_JMP) != ins[9].at ||
        branch_target(ins[7], ZYDIS_MNEMONIC_JMP) != ins[9].at)
        return std::nullopt;
    const auto value = [&](const Instruction &item, std::uint64_t expected) {
        if (item.code.mnemonic != ZYDIS_MNEMONIC_MOV ||
            item.code.operand_count_visible != 2 ||
            item.operands[1].type != ZYDIS_OPERAND_TYPE_IMMEDIATE ||
            item.operands[1].imm.value.u != expected) return false;
        if (stack_form)
            return item.operands[0].type == ZYDIS_OPERAND_TYPE_MEMORY &&
                   item.operands[0].mem.base == ZYDIS_REGISTER_RBP &&
                   item.operands[0].mem.disp.value == -0x78;
        return item.operands[0].type == ZYDIS_OPERAND_TYPE_REGISTER &&
               (item.operands[0].reg.value == ZYDIS_REGISTER_EAX ||
                item.operands[0].reg.value == ZYDIS_REGISTER_EBX);
    };
    if (!value(ins[4], 12) || !value(ins[6], 10) || !value(ins[8], 8) ||
        (!stack_form && (ins[4].operands[0].reg.value != ins[6].operands[0].reg.value ||
                         ins[4].operands[0].reg.value != ins[8].operands[0].reg.value)))
        return std::nullopt;
    if (stack_form && ins[9].code.mnemonic != ZYDIS_MNEMONIC_MOVAPS) return std::nullopt;
    if (!stack_form && ins[9].code.mnemonic != ZYDIS_MNEMONIC_MOV &&
        ins[9].code.mnemonic != ZYDIS_MNEMONIC_CVTTSD2SI) return std::nullopt;
    const auto trio = immediate(ins[4], 32), duo = immediate(ins[6], 32),
               solo = immediate(ins[8], 32);
    if (!trio || !duo || !solo) return std::nullopt;
    return LimitSites{*trio, *duo, *solo};
}

template <std::size_t N> std::optional<std::size_t> wide_text(
    std::span<const std::uint8_t> data, const wchar_t (&text)[N]) {
    std::array<std::uint8_t, N * 2> bytes{};
    for (std::size_t i = 0; i < N; ++i) {
        bytes[2 * i] = static_cast<std::uint8_t>(text[i]);
        bytes[2 * i + 1] = 0;
    }
    std::optional<std::size_t> found;
    for (std::size_t i = 0; i + bytes.size() <= data.size(); i += 2) {
        if (std::memcmp(data.data() + i, bytes.data(), bytes.size()) != 0) continue;
        if (found) throw std::runtime_error("Ambiguous server editor label");
        found = i;
    }
    return found;
}
} // namespace

LimitSites find_limit_sites(std::span<const std::uint8_t> text) {
    ZydisDecoder decoder{};
    if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64,
                                        ZYDIS_STACK_WIDTH_64)))
        throw std::runtime_error("Cannot initialize player-limit decoder");
    std::optional<LimitSites> manager, session;
    for (std::size_t i = 0; i + 48 < text.size(); ++i) {
        if (text[i] == 0x83 && text[i + 1] == 0xe9 && text[i + 2] == 2) {
            if (auto candidate = manager_at(decoder, text, i)) {
                if (manager) throw std::runtime_error("Ambiguous dedicated-manager player limit");
                manager = candidate;
            }
        }
        if (text[i] == 0x41 && text[i + 1] == 0x0f && text[i + 2] == 0xb6) {
            if (auto candidate = session_at(decoder, text, i)) {
                if (session) throw std::runtime_error("Ambiguous EOS-session player limit");
                session = candidate;
            }
        }
    }
    if (!manager || !session)
        throw std::runtime_error("Player-limit native calculations changed or are missing");
    manager->session_trio = session->session_trio;
    manager->session_duo = session->session_duo;
    manager->session_solo = session->session_solo;
    return *manager;
}

std::vector<LimitSites> find_editor_limit_sites(std::span<const std::uint8_t> text) {
    ZydisDecoder decoder{};
    if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LONG_64,
                                        ZYDIS_STACK_WIDTH_64)))
        throw std::runtime_error("Cannot initialize server editor decoder");
    std::vector<LimitSites> found;
    unsigned stack_count{}, register_count{};
    for (std::size_t i = 0; i + 48 < text.size(); ++i) {
        if (text[i] != 0x83 || text[i + 1] != 0xe9 ||
            (text[i + 2] != 1 && text[i + 2] != 2)) continue;
        const bool stack_form = text[i + 2] == 1;
        if (auto sites = editor_at(decoder, text, i, stack_form)) {
            found.push_back(*sites);
            stack_form ? ++stack_count : ++register_count;
        }
    }
    if (stack_count != 1 || register_count != 3)
        throw std::runtime_error("Dedicated server editor mode limits changed or are ambiguous");
    return found;
}

std::size_t find_ui_maximum(std::span<const std::uint8_t> rdata) {
    const auto key = wide_text(rdata, L"MaxPlayers");
    const auto label = wide_text(rdata, L"Max Players");
    if (!key || !label) throw std::runtime_error("Vanilla server editor player field is missing");
    const auto key_pointer = reinterpret_cast<std::uintptr_t>(rdata.data() + *key);
    const auto label_pointer = reinterpret_cast<std::uintptr_t>(rdata.data() + *label);
    std::optional<std::size_t> found;
    for (std::size_t i = 0; i + 48 <= rdata.size(); i += alignof(void *)) {
        std::uintptr_t property{}, title{};
        std::memcpy(&property, rdata.data() + i, sizeof(property));
        std::memcpy(&title, rdata.data() + i + 8, sizeof(title));
        if (property != key_pointer || title != label_pointer) continue;
        double minimum{}, maximum{};
        std::memcpy(&minimum, rdata.data() + i + 32, sizeof(minimum));
        std::memcpy(&maximum, rdata.data() + i + 40, sizeof(maximum));
        if (minimum != 1.0 || maximum != 32.0)
            throw std::runtime_error("Vanilla server editor player range changed");
        if (found) throw std::runtime_error("Ambiguous vanilla server editor player range");
        found = i + 40;
    }
    if (!found) throw std::runtime_error("Vanilla server editor player range not found");
    return *found;
}

} // namespace briefcase::deceive::detail
