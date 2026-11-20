/* Host tooling only: no game state, bytecode input, require resolver or config execution. */
#include "Luau/BuiltinDefinitions.h"
#include "Luau/Error.h"
#include "Luau/FileResolver.h"
#include "Luau/Frontend.h"
#include "Luau/TypeArena.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <vector>

static std::string read_file(const char *path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error(std::string("cannot open ") + path);
    std::string text((std::istreambuf_iterator<char>(input)), {});
    if (input.bad()) throw std::runtime_error(std::string("cannot read ") + path);
    return text;
}
static void json_string(const std::string &text)
{
    static const char hex[] = "0123456789abcdef";
    std::cout << '"';
    for (unsigned char c : text) {
        if (c == '"' || c == '\\') std::cout << '\\' << char(c);
        else if (c < 32) std::cout << "\\u00" << hex[c >> 4] << hex[c & 15];
        else std::cout << char(c);
    }
    std::cout << '"';
}
struct Resolver : Luau::FileResolver {
    std::map<std::string, std::string> sources;
    std::optional<Luau::SourceCode> readSource(const Luau::ModuleName &module) override
    {
        auto found = sources.find(module);
        if (found == sources.end()) return std::nullopt;
        return Luau::SourceCode{found->second, Luau::SourceCode::Script};
    }
};
struct Diagnostic {
    std::string file, message;
    Luau::Location location;
};
static void report(const std::vector<Diagnostic> &diagnostics)
{
    std::cout << "{\"diagnostics\":[";
    for (size_t i = 0; i < diagnostics.size(); ++i) {
        const Diagnostic &d = diagnostics[i];
        if (i) std::cout << ',';
        std::cout << "{\"file\":"; json_string(d.file);
        std::cout << ",\"line\":" << d.location.begin.line + 1
                  << ",\"column\":" << d.location.begin.column + 1
                  << ",\"endLine\":" << d.location.end.line + 1
                  << ",\"endColumn\":" << d.location.end.column + 1
                  << ",\"severity\":\"error\",\"message\":";
        json_string(d.message);
        std::cout << '}';
    }
    std::cout << "],\"ok\":" << (diagnostics.empty() ? "true" : "false") << "}\n";
}
/* usage: p4_luau_check definitions.d.luau mod.luau [more.luau ...]
 * Prints one JSON object per source, one per line in argument order, against
 * one loaded definition file: many checks cost one process (under Wine every
 * process start counts). A usage or I/O error is exactly one object, exit 2;
 * otherwise exit 1 when any source has diagnostics. */
int main(int argc, char **argv)
{
    try {
        if (argc < 3) throw std::runtime_error("usage: p4_luau_check definitions.d.luau mod.luau [more.luau ...]");
        std::string definitions = read_file(argv[1]);
        Resolver resolver;
        for (int i = 2; i < argc; ++i) resolver.sources[argv[i]] = read_file(argv[i]);
        Luau::NullConfigResolver config;
        config.defaultConfig.mode = Luau::Mode::Strict;
        Luau::FrontendOptions options;
        options.moduleTimeLimitSec = 10;
        Luau::Frontend frontend(Luau::SolverMode::New, &resolver, &config, options);
        Luau::registerBuiltinGlobals(frontend, frontend.globals);
        auto loaded = frontend.loadDefinitionFile(frontend.globals, frontend.globals.globalScope,
                                                   definitions, argv[1], false);
        Luau::freeze(frontend.globals.globalTypes);
        std::vector<Diagnostic> rejected;
        for (const auto &error : loaded.parseResult.errors)
            rejected.push_back({argv[1], error.getMessage(), error.getLocation()});
        if (loaded.module) for (const auto &error : loaded.module->errors)
            rejected.push_back({argv[1], Luau::toString(error), error.location});
        if (!loaded.success && rejected.empty()) rejected.push_back({argv[1], "definition file rejected", {}});
        int status = 0;
        for (int i = 2; i < argc; ++i) {
            std::vector<Diagnostic> diagnostics = rejected;
            if (loaded.success) {
                auto result = frontend.check(argv[i]);
                for (const auto &error : result.errors)
                    diagnostics.push_back({error.moduleName, Luau::toString(error), error.location});
                for (const auto &name : result.timeoutHits)
                    diagnostics.push_back({name, "typecheck time limit exceeded", {}});
            }
            report(diagnostics);
            if (!diagnostics.empty()) status = 1;
        }
        return status;
    } catch (const std::exception &error) {
        report({{argc > 2 ? argv[2] : "", error.what(), {}}});
        return 2;
    }
}
