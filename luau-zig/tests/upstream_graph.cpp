// Test-only validation of compiler-produced function bytecode with the pinned
// upstream graph parser/serializer used by Luau.Conformance.
#include "Luau/BytecodeBuilder.h"
#include "Luau/BytecodeGraph.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

LUAU_FASTFLAG(LuauEmitCallFeedback)
LUAU_FASTFLAG(LuauCompileCallFbRemap)

struct Reader {
    const std::string& data;
    size_t pos = 0;
    uint8_t byte() {
        if (pos == data.size()) throw std::runtime_error("truncated bytecode");
        return uint8_t(data[pos++]);
    }
    uint32_t varint() {
        uint32_t value = 0;
        for (unsigned shift = 0; shift < 35; shift += 7) {
            uint8_t b = byte();
            if (shift == 28 && (b & 0xf0)) throw std::runtime_error("varint overflow");
            value |= uint32_t(b & 0x7f) << shift;
            if (!(b & 0x80)) return value;
        }
        throw std::runtime_error("unterminated varint");
    }
    std::string_view take(size_t size) {
        if (size > data.size() - pos) throw std::runtime_error("truncated bytecode payload");
        std::string_view result(data.data() + pos, size);
        pos += size;
        return result;
    }
};

int main(int argc, char** argv)
{
    try {
        bool requireCallFeedback = argc == 3 && std::strcmp(argv[2], "--require-callfb") == 0;
        bool requireFastcall = argc == 4 && std::strcmp(argv[2], "--require-fastcall-id") == 0;
        bool forbidFastcall = argc == 4 && std::strcmp(argv[2], "--forbid-fastcall-id") == 0;
        if (argc != 2 && !requireCallFeedback && !requireFastcall && !forbidFastcall)
            throw std::runtime_error("expected a bytecode file and optional graph requirement");
        int fastcallId = (requireFastcall || forbidFastcall) ? std::stoi(argv[3]) : -1;
        if ((requireFastcall || forbidFastcall) && (fastcallId < 1 || fastcallId > 134 || std::to_string(fastcallId) != argv[3]))
            throw std::runtime_error("invalid fastcall id");
        unsigned fastcallCount = 0;
        unsigned matchingFastcalls = 0;
        bool foundCallFeedback = false;
        if (requireCallFeedback) {
            FFlag::LuauEmitCallFeedback.value = true;
            FFlag::LuauCompileCallFbRemap.value = true;
        }
        std::ifstream input(argv[1], std::ios::binary);
        if (!input) throw std::runtime_error("cannot open bytecode file");
        std::string bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        Reader reader{bytes};
        uint8_t version = reader.byte();
        if (version < 12) throw std::runtime_error("bytecode lacks length-framed functions");
        uint8_t typesVersion = reader.byte();
        if (typesVersion != 3) throw std::runtime_error("unexpected bytecode type version");
        uint32_t stringCount = reader.varint();
        if (stringCount > bytes.size()) throw std::runtime_error("invalid string count");
        std::vector<std::string_view> strings;
        strings.reserve(stringCount);
        for (uint32_t i = 0; i < stringCount; ++i)
            strings.push_back(reader.take(reader.varint()));
        while (reader.byte() != 0)
            if (reader.varint() > stringCount) throw std::runtime_error("invalid userdata type name");
        uint32_t functionCount = reader.varint();
        if (!functionCount || functionCount > bytes.size()) throw std::runtime_error("invalid function count");
        Luau::BytecodeBuilder reserialized;
        for (uint32_t i = 0; i < functionCount; ++i) {
            std::string_view function = reader.take(reader.varint());
            if (auto graph = Luau::Bytecode::fromFunctionBytecode(std::string(function), strings)) {
                for (const auto& instruction : graph->instructions) {
                    foundCallFeedback |= instruction.op == LOP_CALLFB;
                    if (instruction.op == LOP_FASTCALL || instruction.op == LOP_FASTCALL1 ||
                        instruction.op == LOP_FASTCALL2 || instruction.op == LOP_FASTCALL2K ||
                        instruction.op == LOP_FASTCALL3) {
                        ++fastcallCount;
                        if (!instruction.ops.empty() && instruction.ops[0].kind == Luau::Bytecode::BcOpKind::Imm) {
                            const auto& value = graph->immediates[instruction.ops[0].index];
                            if (value.kind == Luau::Bytecode::BcImmKind::Int && value.valueInt == fastcallId)
                                ++matchingFastcalls;
                        }
                    }
                }
                if (Luau::Bytecode::toFunctionBytecode(reserialized, *graph).empty())
                    throw std::runtime_error("graph serialization failed");
                reserialized.clearStrings();
            }
            else if (requireFastcall || forbidFastcall)
                throw std::runtime_error("cannot inspect a function graph");
        }
        if (requireCallFeedback && !foundCallFeedback)
            throw std::runtime_error("compiler output has no CALLFB instruction");
        if (requireFastcall && (fastcallCount != 1 || matchingFastcalls != 1))
            throw std::runtime_error("expected exactly one fastcall to the requested builtin");
        if (forbidFastcall && matchingFastcalls != 0)
            throw std::runtime_error("unexpected fastcall for disabled builtin");
        if (reader.varint() >= functionCount || reader.pos != bytes.size())
            throw std::runtime_error("invalid main function or trailing data");
        std::cout << "OK\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
