--!capability mods.required@1
-- p4.mods.required(id): the admitted version and current required settings of a package the mod names in [require].
--!in p4.mods
-- A declared [require] package: its admitted version and the current
-- values of the settings this mod requires of it. Undeclared ids error.
required: (id: string) -> { version: string, settings: { [string]: boolean | string } },
--!example
local base = p4.mods.required("p4pc.base")
local base_version: string, split = base.version, base.settings.split
--!refuse
local v: number = p4.mods.required("p4pc.base").version
p4.mods.required(7)
