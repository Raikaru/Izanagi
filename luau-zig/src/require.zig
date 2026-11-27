//! Require-by-string for the Zig Luau engine: the upstream `Luau.Require` runtime
//! library, its `.luaurc` / `.config.luau` configuration support, and the `luau` CLI's
//! filesystem configuration, translated to Zig (no C/C++ Luau code is linked).
//!
//! Translation of upstream Luau 0.744 (commit 32c28089305f488f901aaf1dbf3fd1cb65fa3d11):
//! Require/ (Require.cpp, RequireImpl.cpp, Navigation.cpp, RequireNavigator.cpp,
//! PathUtilities.cpp, AliasCycleTracker.cpp), Config/ (Config.cpp, LuauConfig.cpp,
//! LinterConfig.cpp) and CLI/ (ReplRequirer.cpp, VfsNavigator.cpp, FileUtils.cpp,
//! the require setup in Repl.cpp).
//!
//! Portions derived from Luau, distributed under the MIT license:
//!
//! Copyright (c) 2019-2026 Roblox Corporation
//!
//! Permission is hereby granted, free of charge, to any person obtaining a copy of
//! this software and associated documentation files (the "Software"), to deal in
//! the Software without restriction, including without limitation the rights to
//! use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
//! the Software, and to permit persons to whom the Software is furnished to do so,
//! subject to the following conditions:
//!
//! The above copyright notice and this permission notice shall be included in all
//! copies or substantial portions of the Software.
//!
//! THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//! IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
//! FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
//! COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
//! IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//! CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//!
//! This is the CLI facility, unrelated to the game's restricted mod sandbox
//! (`stdlib.p4pc_openlibs_sandbox`), which has no `require`.
//!
//! ## CLI integration (upstream Repl.cpp)
//!
//! ```zig
//! const require = luau.require;
//! require.flags.setAll(true);              // setLuauFlagsDefault() turns these on
//! libs.luaL_openlibs(L);
//! // ... register loadstring / collectgarbage ...
//! _ = require.cli.open(L, io, gpa, &host.options); // global `require`
//! libs.luaL_sandbox(L);
//! // scripts: chunkname "@" ++ normalizePath(file), on a luaL_sandboxthread'ed thread
//! // REPL:    chunkname "=stdin"
//! ```
//! `require` resolves relative to the requiring chunk's name, so the host must name
//! script chunks `"@" ++ require.file_utils.normalizePath(path)` (runFile) and REPL
//! input `"=stdin"`; any other chunk name cannot require.
//!
//! ## Embedding with a custom resolver
//!
//! Fill a `Configuration` (luarequire_Configuration with Zig callbacks) and call
//! `library.open` / `library.pushRequire` / `library.pushProxyRequire`. The cache and
//! registration helpers (`registerModule`, `clearCacheEntry`, `clearCache`) are
//! `lua_CFunction`s operating on their stack arguments, as upstream.
//!
//! ## Errors
//!
//! Resolution failures raise `error requiring module "<path>": <reason>` (reasons as
//! upstream, e.g. `could not resolve child component "x"`, `@x is not a valid alias`,
//! `detected alias cycle (@a -> @b -> @a)`, configuration parse/execution messages);
//! other failures raise the upstream messages listed in `cli.zig` and `library.zig`.
//! Running out of memory in Zig-side resolution raises "not enough memory".

pub const flags = @import("require/flags.zig");
pub const library = @import("require/library.zig");
pub const navigation = @import("require/navigation.zig");
pub const config = @import("require/config.zig");
pub const luau_config = @import("require/luau_config.zig");
pub const file_utils = @import("require/file_utils.zig");
pub const vfs_navigator = @import("require/vfs_navigator.zig");
pub const cli = @import("require/cli.zig");

pub const Configuration = navigation.Configuration;
pub const NavigateResult = navigation.NavigateResult;
pub const ConfigStatus = navigation.ConfigStatus;

pub const open = library.open;
pub const pushRequire = library.pushRequire;
pub const pushProxyRequire = library.pushProxyRequire;
pub const registerModule = library.registerModule;
pub const clearCacheEntry = library.clearCacheEntry;
pub const clearCache = library.clearCache;
pub const lockPlaceholder = library.lockPlaceholder;
pub const populatePlaceholder = library.populatePlaceholder;
pub const createPlaceholder = library.createPlaceholder;

test {
    @import("std").testing.refAllDecls(@This());
    _ = file_utils;
    _ = config;
    _ = navigation;
}
