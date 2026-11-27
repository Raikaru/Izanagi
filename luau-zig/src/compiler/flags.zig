//! Upstream Luau feature flags that affect parsing, compilation or bytecode
//! serialization, mirrored from the LUAU_FASTFLAGVARIABLE declarations in
//! third_party/luau/{Ast,Compiler,Bytecode}/src (Luau, MIT License; see
//! third_party/luau/LICENSE.txt). Every flag defaults to off, exactly like the
//! pinned upstream build. Field names keep the upstream flag spelling so test
//! harnesses can map ScopedFastFlag settings 1:1.
//!
//! All flags are implemented with their upstream semantics. In the pinned
//! sources LuauSolverV2, LuauAllowGlobalDeclarationToBeCalledClass (declaration
//! syntax is never enabled by the compiler), DebugLuauReportReturnTypeVariadicWithTypeSuffix
//! (telemetry only) and LuauBytecodeCostModel (flag version metadata only) have no
//! observable effect on the produced bytecode.

pub const Flags = struct {
    // Ast/src/Parser.cpp
    LuauSolverV2: bool = false,
    LuauIntegerType2: bool = false,
    LuauExportValueSyntax: bool = false,
    DebugLuauNoInline: bool = false,
    DebugLuauUserDefinedClasses: bool = false,
    LuauAllowGlobalDeclarationToBeCalledClass: bool = false,
    LuauSingleTypeOptionalPackReturnsAttributeParens: bool = false,
    DebugLuauParseExactTables: bool = false,
    LuauExperimentalIfLocalSyntax: bool = false,
    DebugLuauReportReturnTypeVariadicWithTypeSuffix: bool = false,

    // Compiler/src/Builtins.cpp
    LuauIntegerFastcalls: bool = false,
    LuauIntegerBufferFastcalls: bool = false,
    LuauCompileTableBuiltinsCreateClear: bool = false,

    // Compiler/src/Compiler.cpp
    LuauCompileMoveElision: bool = false,
    LuauCompileRemoveStatLocalLoop: bool = false,
    LuauCompileMoveElisionFix: bool = false,
    LuauCompileNoDebugConst: bool = false,
    LuauCompileConcatTargetTop: bool = false,
    LuauCompileReuseLowersLimit: bool = false,
    LuauOptimizeExportTable: bool = false,
    LuauExportedTypesParticipateInScc: bool = false,
    LuauCompileReuseLocalRegs: bool = false,

    // Compiler/src/ConstantFolding.cpp
    LuauCompileNoFoldVectorEqW: bool = false,
    LuauCompileTableFoldFix: bool = false,

    // Bytecode/src/BytecodeBuilder.cpp
    LuauEmitCallFeedback: bool = false,
    LuauCompileRefactorFeedback: bool = false,
    LuauBytecodeCostModel: bool = false,
    LuauCompileCallFbRemap: bool = false,
    LuauCompileFastpcall: bool = false,
};
