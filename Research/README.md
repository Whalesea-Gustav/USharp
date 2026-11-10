# BMW runtime reconstruction prototype

Base: pixeltris/USharp commit 8f0205cd8628b84a66326947d93588a8c8dcbd24.
Branch: codex/bmw-runtime-prototype.

This reference checkout is not installed into WuKongUE. It is a reconstruction
experiment, not the recovered native BMW plugin.

## Implemented

- SetFunctionInvoker now accepts an explicit managed invoker in native export,
  managed delegate and the existing re-registration caller. This matches the
  observed BMW parameter shape.
- REA/Ghidra and exact x64 instructions at 0x14ae5b390 show that BMW writes the
  supplied pointer directly to three locations. The helper now reproduces that
  behavior, including null-pointer writes; invented validation was removed.
- Upstream native function-dispatch thunk and per-class invoker storage were
  removed. Managed dispatch uses BMW's global UFunction-address-to-owner map.
  Initial function registration also installs the managed static dispatcher.
- Managed constructors now receive class address and object initializer, as
  shown by recovered C# and the retained native constructor thunk evidence.
  Non-AOT construction is wired; full AOT backend selection is still pending.
- The earlier prototype-only capability export was removed to match the
  observed BMW interface. Native and managed binaries must be built together.
- GameThreadDefer now has native exports, a shared evidence-based queue core,
  managed ownership-flag wrappers and recovered generator whitelist/gating.
  Parameter buffers are shallow-copied; deferred callers relinquish destruction
  and Flush destroys eligible parameters after ProcessEvent. No AsyncTask is used.

## Validation

Run Research/run-validation.ps1 from PowerShell on this workstation. Toolchain
paths are intentionally local and explicit.

The complete managed runtime and untouched baseline both build with the command
line target override TargetFrameworkVersion=v4.8. Both report the same eight
CS8500 warnings in upstream Marshalers.cs. The original v4.5.2 attempt failed
because the required reference assemblies were absent. The csproj's declared
framework is unchanged; v4.8 compilation does not prove v4.5.2 compatibility.

The native standalone test includes the actual installation helper used by the
export and passes 14 checks using mock UE tables/functions. It does not compile
the full Export_USharpClass.h or execute Unreal. Reflection checks on the built
runtime confirm the three-argument delegate, two-argument constructor callback,
absence of upstream-only invoker exports and the static dispatcher. An isolated
synthetic-frame test checks missing, registered and cleared owner-map entries;
its final SkipFunction boundary is mocked, not executed in UE.

Run Research/verify-bmw-invoker-evidence.py to recheck the original executable
digest and retained native instruction bytes. REA evidence is preserved in
ResearchArtifacts/BmwInvoker; final build/test outputs are in
ResearchArtifacts/FaithfulInvoker. Older first-prototype outputs remain
historical and do not describe the current implementation.

ResearchArtifacts contains local build logs and binaries. They are not a
deployable plugin.

Research/run-defer-validation.ps1 is the combined local runner. The standalone
queue contract passes 15 checks, the actual managed DLL passes defer return/flag
checks, and Research/UE/USharpQueueTest passes 7 checks in UE5.4.4 DebugGame Editor.
That project loads only the shared queue core and a UE5 FProperty adapter; it
does not load the complete USharp plugin or CLR. Its Saved/queue-evidence.json
records three events, one parameter destructor and 20 copied payload bytes.
The source binary digest and canonical REA records are rechecked by
Research/verify-bmw-defer-evidence.py, including the short relocation-copy size.

## Remaining work

Native UE4.23-to-UE5 reflection migration and full plugin compilation remain
open. GameThreadDefer preserves the observed raw object/function pointers; this
scope found no queue-owned UObject roots. Original growth arithmetic uses a
byte capacity and Count*8 addressing but copies only Count bytes on relocation.
A padded synthetic allocator reproduces lost record bytes; original-game
occurrence is unproven and large production queues are not accepted. Pending
shutdown, GC, recursive flush and full ECS integration remain unresolved.
Generated code compiles as generator source, but has not run end-to-end through
the complete native USharp runtime. AOT/IL2CPP GC and thread
attachment require a real matching runtime backend; no success-returning stubs
are provided. The queue-core UE test is narrower than full plugin acceptance.
Exception reporting still uses the upstream handler and FFrame remains an
upstream wrapper rather than the recovered raw struct. These are open fidelity
differences; the complete plugin is not yet equivalent to BMW.
