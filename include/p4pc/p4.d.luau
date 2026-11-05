-- API1.4. Luau doubles/bit32/public buffers; never Lua5.4 integers/bytecode.
-- Guest addresses are32-bit values, not host pointers. i64/u64 are8-byte LE
-- buffers; p4.u64/split64 preserve all bits without numeric64-bit coercion.
-- Namespace/handle properties and event records are readonly at runtime.
-- Match the provider's actual opened libraries, not Analysis' larger builtin
-- catalog. Ordinary pcall/xpcall/getfenv/setfenv/coroutines remain available.
declare os: nil
declare io: nil
declare debug: nil
declare package: nil
declare require: nil
declare load: nil
declare loadstring: nil
declare loadfile: nil
declare dofile: nil
declare collectgarbage: nil
declare vector: nil
declare integer: nil
declare class: nil
declare types: nil

export type P4NumberKind = "u8" | "u16" | "u32" | "i8" | "i16" | "i32" | "f32" | "f64"
export type P4WideKind = "u64" | "i64"
export type P4ScalarKind = P4NumberKind | P4WideKind | "fn"
export type P4Scalar = number | buffer
export type P4FunctionRef = { address: number? }
export type P4GuestBuffer = {
    address: number,
    size: number,
    release: (self: P4GuestBuffer) -> (),
}
export type P4Address = number | P4GuestBuffer
export type P4GuestArray<T> = { count: number, [number]: T }
export type P4Layout<T> = {
    name: string,
    size: number,
    view: (self: P4Layout<T>, address: P4Address, readonly: boolean?) -> T & { address: number, layout: P4Layout<T> },
}
export type P4FieldSpec = {
    offset: number,
    type: P4ScalarKind | "string" | P4Layout<any>,
    count: number?,
    capacity: number?,
    readonly: boolean?,
}
export type P4LayoutSpec = { name: string, size: number, fields: { [string]: P4FieldSpec } }
-- Plain acyclic tables use only boolean/number/string keys and values recursively.
-- Binary strings retain their exact bytes; native buffers/handles are not values.
export type P4SaveValue = nil | boolean | number | string | { [boolean | number | string]: P4SaveValue }
export type P4SaveSpec = {
    schema: number,
    max_bytes: number,
    snapshot: () -> P4SaveValue,
    load: (value: P4SaveValue, origin: "load" | "reset") -> (),
    -- Snapshot-mode (no native calls/guest mutation); false/nil refuses.
    authorize_load: ((value: P4SaveValue) -> (boolean, ...string))?,
    authorize_save: (() -> (boolean, ...string))?,
    -- After a save carrying this block committed durably; next frame boundary.
    published: ((value: P4SaveValue) -> ())?,
}
export type P4Subscription = {
    enable: (self: P4Subscription) -> (),
    disable: (self: P4Subscription) -> (),
    remove: (self: P4Subscription) -> (),
}
export type P4HookContext = {
    address: number,
    signature: string,
    phase: "before" | "after" | "replace",
    args: { P4Scalar },
    result: P4Scalar?,
    next: () -> P4Scalar?,
}
export type P4FunctionInfo = { address: number, signature: string, hooked: boolean }
export type P4FrameEvent = { name: "frame_start", frame: number }
export type P4FieldEnterEvent = { name: "field_enter", task: number, field: number, room: number, date: number, period: number }
export type P4FieldLeaveEvent = { name: "field_leave", task: number, field: number, room: number, date: number, period: number }
export type P4BattleStartEvent = { name: "battle_start", task: number, setup: number, battle: number }
export type P4BattleEndEvent = { name: "battle_end", battle: number, outcome: number }
export type P4MenuOpenEvent = { name: "menu_open", task: number, kind: number }
export type P4MenuCloseEvent = { name: "menu_close", task: number, kind: number }
export type P4CalendarEvent = { name: "calendar_advance", old_date: number, old_period: number, date: number, period: number }
export type P4SaveEvent = { name: "save", version: number, data: number, size: number }
export type P4LoadEvent = { name: "load", version: number, data: number, size: number }
export type P4ExitEvent = { name: "exit" }
-- kind: 0 pair, 1 triangle, 2..4 cross spread (4/5/6 materials), 5 twelve-material
export type P4FusionContext = { name: "fusion", kind: number, materials: number }
export type P4EncounterContext = { name: "encounter", encounter: number, source: number }
export type P4FormationContext = { name: "formation", party: {number} }
export type P4InputAction = "confirm" | "cancel" | "menu" | "help" | "cross" | "circle" | "triangle" | "square"
    | "up" | "down" | "left" | "right" | "start" | "select" | "l1" | "r1" | "l2" | "r2" | "l3" | "r3"
-- API 1.4 game services. Requests are queued and run at the next frame boundary.
export type P4SceneState = {
    context: "none" | "title" | "field" | "dungeon" | "battle" | "event" | "other",
    sequence: number, ready: boolean, reason: string?,
    control: boolean, event: boolean, menu: boolean, pending: boolean,
    field: number, room: number, entry: number?, step: number?,
}
export type P4WarpSpec = { field: number, room: number, entry: number?, direction: number? }
export type P4RequestState = "idle" | "requested" | "battle" | "returning" | "done" | "refused"
export type P4EncounterInfo = { id: number, enemies: { number }, formation: number, variant: number }
export type P4CameraPose = { x: number, y: number, z: number, yaw: number, pitch: number, fov: number }
export type P4CameraLease = {
    set: (self: P4CameraLease, pose: { x: number?, y: number?, z: number?, yaw: number?, pitch: number?, fov: number? }) -> boolean,
    get: (self: P4CameraLease) -> P4CameraPose?,
    active: (self: P4CameraLease) -> (boolean, string?),
    input: (self: P4CameraLease, exclusive: boolean) -> boolean,
    release: (self: P4CameraLease) -> (),
}
export type P4HudGroup = "field_banner" | "field_map" | "calendar" | "party" | "battle_commands"

export type P4UIRow = {
    key: string, label: string,
    kind: "info" | "toggle" | "value" | "action" | "link",
    value: string?, help: string?, target: string?,
    checked: boolean?, disabled: boolean?, restart: boolean?, current: boolean?,
}
export type P4UIPage = {
    key: string, -- readonly namespaced key; open accepts a request, not visibility
    open: (self: P4UIPage) -> boolean,
    refresh: (self: P4UIPage) -> boolean,
}
export type P4UIPageSpec = {
    key: string, title: string,
    listed: boolean?, -- API 1.4: false = reachable by link rows only, not a hub link
    count: () -> number,
    row: (index: number) -> P4UIRow, -- one-based
    find: (key: string) -> number?, -- one-based; nil if removed
    adjust: ((key: string, direction: number) -> ())?,
    opened: (() -> ())?, closed: (() -> ())?,
}
export type P4UIDetailSpec = {
    key: string, title: string, detail: () -> string,
    listed: boolean?,
    opened: (() -> ())?, closed: (() -> ())?,
}

declare p4: {
    api_version: string,
    language: "Luau",
    language_version: "0.744",
    id: string,
    log: (message: string) -> (),
    u64: (high: number, low: number) -> buffer,
    split64: (value: buffer) -> (number, number),
    fn: (retail_address: number) -> P4FunctionRef?,
    describe: (address: number | P4FunctionRef) -> P4FunctionInfo?,
    call: (address: number | P4FunctionRef, signature: string, ...P4Scalar) -> P4Scalar?,
    hook: (address: number | P4FunctionRef, signature: string, phase: "before" | "after" | "replace",
        callback: (context: P4HookContext) -> ()) -> P4Subscription,
    mem: {
        alloc: (size: number) -> P4GuestBuffer,
        sizeof: (kind: P4ScalarKind) -> number,
        read: ((kind: P4NumberKind, address: P4Address) -> number)
            & ((kind: P4WideKind, address: P4Address) -> buffer)
            & ((kind: "fn", address: P4Address) -> P4FunctionRef?),
        write: ((kind: P4NumberKind, address: P4Address, value: number) -> ())
            & ((kind: P4WideKind, address: P4Address, value: buffer) -> ())
            & ((kind: "fn", address: P4Address, value: number | P4FunctionRef?) -> ()),
        read_string: (address: P4Address, capacity: number) -> string,
        write_string: (address: P4Address, capacity: number, value: string) -> (),
        read_bytes: (address: P4Address, size: number) -> buffer,
        write_bytes: (address: P4Address, value: buffer) -> (),
        layout: <T>(specification: P4LayoutSpec) -> P4Layout<T>,
    },
    events: {
        -- Separate phantom generics keep Luau Analysis from exponentially
        -- saturating this intersection; each signature remains a real function.
        on: (<T>(name: "frame_start", callback: (event: P4FrameEvent) -> ()) -> P4Subscription)
            & (<T>(name: "field_enter", callback: (event: P4FieldEnterEvent) -> ()) -> P4Subscription)
            & (<T>(name: "field_leave", callback: (event: P4FieldLeaveEvent) -> ()) -> P4Subscription)
            & (<T>(name: "battle_start", callback: (event: P4BattleStartEvent) -> ()) -> P4Subscription)
            & (<T>(name: "battle_end", callback: (event: P4BattleEndEvent) -> ()) -> P4Subscription)
            & (<T>(name: "menu_open", callback: (event: P4MenuOpenEvent) -> ()) -> P4Subscription)
            & (<T>(name: "menu_close", callback: (event: P4MenuCloseEvent) -> ()) -> P4Subscription)
            & (<T>(name: "calendar_advance", callback: (event: P4CalendarEvent) -> ()) -> P4Subscription)
            & (<T>(name: "save", callback: (event: P4SaveEvent) -> ()) -> P4Subscription)
            & (<T>(name: "load", callback: (event: P4LoadEvent) -> ()) -> P4Subscription)
            & (<T>(name: "exit", callback: (event: P4ExitEvent) -> ()) -> P4Subscription),
    },
    input: {
        action: (action: P4InputAction) -> (boolean, boolean, boolean),
        sticks: () -> (number, number, number, number),
    },
    files: {
        read: (path: string, maximum: number?) -> buffer,
    },
    settings: {
        -- Declared toggles read as booleans and choices as strings; undeclared
        -- data keys return the stored scalar or nil.
        get: (key: string) -> (boolean | number | string)?,
        set: (key: string, value: (boolean | number | string)?) -> (boolean, string?),
    },
    -- Port session identity and the crash handler's marker (API 1.3).
    lifecycle: {
        session: () -> (number?, string?),
        tag: (tag: string?) -> (boolean, string?),
        -- true (and this mod's tag) when the port's crash handler recorded a
        -- crash in that session, false when it did not, nil and a reason when
        -- its marker is unreadable.
        crashed: (session: number) -> (boolean?, string?),
        consume: (session: number) -> (boolean, string?),
    },
    save: {
        register: (specification: P4SaveSpec) -> (),
    },
    ui: {
        page: (specification: P4UIPageSpec) -> P4UIPage,
        detail: (specification: P4UIDetailSpec) -> P4UIPage,
        back: () -> boolean, close: () -> boolean, active: () -> string?,
        status: (text: (string | {string})?, placement: ("camp" | "hud")?) -> boolean,
    },
    -- Snapshot-mode callbacks (no native calls/mutation); false denies.
    authorize: {
        on: ((operation: "fusion", callback: (context: P4FusionContext) -> (boolean, ...string)) -> P4Subscription)
            & ((operation: "encounter", callback: (context: P4EncounterContext) -> (boolean, ...string)) -> P4Subscription),
    },
    -- One projector at a time; snapshot-mode; nil keeps the battle's lineup.
    battle: {
        formation: (callback: (context: P4FormationContext) -> {number}?) -> P4Subscription,
    },
    -- wall: host monotonic seconds (presentation/timers only, not deterministic);
    -- updates/retail: deterministic game counters.
    clock: { wall: () -> number, updates: () -> number, retail: () -> number },
    scene: {
        state: () -> P4SceneState,
        available: (field: number, room: number) -> boolean,
        warp: (spec: P4WarpSpec) -> (boolean, string?),
        status: () -> (P4RequestState, string, number?),
    },
    encounter: {
        count: () -> number,
        info: (id: number) -> P4EncounterInfo?,
        launch: (id: number) -> (boolean, string?),
    },
    -- Render-only view override; (nil, reason, owner) when refused.
    camera: { acquire: (name: string) -> (P4CameraLease?, string?, string?), owner: () -> string? },
    hud: {
        groups: () -> { P4HudGroup },
        hide: (group: P4HudGroup, hidden: boolean) -> (boolean, string?),
        hidden: (group: P4HudGroup) -> boolean,
    },
}
