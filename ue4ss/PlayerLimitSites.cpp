#include "PlayerLimitSites.hpp"

#include <Zydis/Zydis.h>
#include <array>
#include <optional>
#include <stdexcept>

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

} // namespace briefcase::deceive::detail
