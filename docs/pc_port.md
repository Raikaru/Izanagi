# Platforms and word size

Izanagi supports Windows x64, Linux x86_64 and aarch64, macOS arm64 and x86_64,
and Android arm64. See [BUILDING.md](../BUILDING.md) for the supported build
paths and [README.md](../README.md#install) for available packages. i386 builds
are retained for development and testing; they are not the default player.

The game uses 32-bit guest addresses even on 64-bit hosts. Guest records and
serialized data retain their original layouts, while native code translates
guest addresses to host pointers at the boundary. See [the 64-bit port model](port64.md)
and [contribution rules](../CONTRIBUTING.md) before changing those interfaces.

The decompilation is a pinned submodule. The build rewrites a generated mirror
rather than editing the submodule. Overrides are maintained once and compiled
for the supported host word sizes.

The renderer defaults to OpenGL ES 3.0; desktop OpenGL 3.3 core is selectable.
See [renderer details](renderer_port.md) and [platform contracts](platform.md).
