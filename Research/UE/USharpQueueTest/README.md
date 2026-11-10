# USharp BMW queue test project

This independent UE5.4 Editor project compiles the same BmwGameThreadDeferQueue.h
used by the reference USharp plugin. Its FProperty adapter executes actual
UObject::ProcessEvent and parameter destruction. It does not load the full
UE4.23 USharp plugin or a CLR.

Run Research/run-defer-validation.ps1 in the reference plugin checkout.
The project disables shared PCHs to avoid long Windows intermediate paths.
Saved/queue-evidence.json records the checks; Saved/queue-editor.log is the
first successful Editor run. Earlier failed build logs are retained.

The commandlet tests immediate game-thread execution, delayed worker calls,
copied integer parameters after sender mutation, shallow FString ownership
transfer, ordered flush, destructor filtering and an empty second flush.
The test host keeps its receiver alive; the queue itself retains raw pointers,
as observed in the scoped original enqueue path.

Growth-unit behavior, concurrent teardown, raw-object validity and recursive
flush are not accepted here. The standalone padded-memory test separately
documents the preserved legacy relocation behavior without executing invalid
function pointers.
