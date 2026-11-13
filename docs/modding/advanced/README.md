# Advanced modding

[Modding home](../README.md)

These interfaces are for work that needs engine internals. They are not needed
for the [first mod](../start/first-mod.md). Native addresses, raw row bytes and
memory layouts are build-specific and can conflict or break with port updates.
They are not a replacement for the planned semantic APIs.

- [Raw hooks and calls](raw-luau.md#typed-hooks-at-retail-addresses)
- [Guest memory and layouts](raw-luau.md#guest-memory-function-words-and-declared-layouts)
  and the [memory reference](../memory.md)
- [Declarative table-source schema](../tables.md#declarative-table-source-schema)
  and [table implementation details](../tables.md)
- [Package internals and VFS provenance](../files.md#shared-native-registry-and-config-discovery)
- [Budgets and qualification](../budgets.md)

Do not distribute game data, extracted tables or disc assets. A synthetic
fixture pass is not proof of gameplay or rendering; the individual references
state what each witness actually covers.
