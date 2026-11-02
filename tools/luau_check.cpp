/* Host tooling only: no game state, bytecode input, require resolver or config execution. */
#include "Luau/BuiltinDefinitions.h"
#include "Luau/Error.h"
#include "Luau/FileResolver.h"
#include "Luau/Frontend.h"
#include "Luau/TypeArena.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

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
    std::string name, source;
    std::optional<Luau::SourceCode> readSource(const Luau::ModuleName &module) override
    {
        if (module != name) return std::nullopt;
        return Luau::SourceCode{source, Luau::SourceCode::Script};
    }
};
static void diagnostic(const std::string &file, const Luau::Location &location,
                       const std::string &message, bool &comma)
{
    if (comma) std::cout << ',';
    comma = true;
    std::cout << "{\"file\":"; json_string(file);
    std::cout << ",\"line\":" << location.begin.line + 1
              << ",\"column\":" << location.begin.column + 1
              << ",\"endLine\":" << location.end.line + 1
              << ",\"endColumn\":" << location.end.column + 1
              << ",\"severity\":\"error\",\"message\":";
    json_string(message);
    std::cout << '}';
}
int main(int argc, char **argv)
{
    bool comma = false;
    /* Every outcome, including I/O and usage errors, is exactly one JSON object. */
    std::cout << "{\"diagnostics\":[";
    try {
        if (argc != 3) throw std::runtime_error("usage: p4_luau_check definitions.d.luau mod.luau");
        std::string definitions = read_file(argv[1]);
        Resolver resolver;
        resolver.name = argv[2];
        resolver.source = read_file(argv[2]);
        Luau::NullConfigResolver config;
        config.defaultConfig.mode = Luau::Mode::Strict;
        Luau::FrontendOptions options;
        options.moduleTimeLimitSec = 10;
        Luau::Frontend frontend(Luau::SolverMode::New, &resolver, &config, options);
        Luau::registerBuiltinGlobals(frontend, frontend.globals);
        auto loaded = frontend.loadDefinitionFile(frontend.globals, frontend.globals.globalScope,
                                                   definitions, argv[1], false);
        Luau::freeze(frontend.globals.globalTypes);
        for (const auto &error : loaded.parseResult.errors)
            diagnostic(argv[1], error.getLocation(), error.getMessage(), comma);
        if (loaded.module) for (const auto &error : loaded.module->errors)
            diagnostic(argv[1], error.location, Luau::toString(error), comma);
        if (!loaded.success && !comma)
            diagnostic(argv[1], {}, "definition file rejected", comma);
        if (loaded.success) {
            auto result = frontend.check(resolver.name);
            for (const auto &error : result.errors)
                diagnostic(error.moduleName, error.location, Luau::toString(error), comma);
            for (const auto &name : result.timeoutHits)
                diagnostic(name, {}, "typecheck time limit exceeded", comma);
        }
        std::cout << "],\"ok\":" << (comma ? "false" : "true") << "}\n";
        return comma ? 1 : 0;
    } catch (const std::exception &error) {
        diagnostic(argc > 2 ? argv[2] : "", {}, error.what(), comma);
        std::cout << "],\"ok\":false}\n";
        return 2;
    }
}
