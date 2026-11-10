#pragma once

// Shared with the standalone contract tests; callers serialize class registration.
template <typename ClassType, typename NameType, typename InvokerType>
auto InstallSharpFunctionInvoker(ClassType* Instance, const NameType& Name,
	InvokerType Invoker)
	-> decltype(Instance->FuncMapEx.Find(Name)->Function)
{
	if (Instance->FuncMapEx.Num() != Instance->NativeFunctionLookupTable.Num())
	{
		return nullptr;
	}
	auto* Entry = Instance->FuncMapEx.Find(Name);
	if (!Entry || Entry->Index < 0)
	{
		return nullptr;
	}
	auto& NativeEntry = Instance->NativeFunctionLookupTable[Entry->Index];
	Entry->Pointer = Invoker;
	Entry->Function->SetNativeFunc(Invoker);
	NativeEntry.Pointer = Invoker;
	return Entry->Function;
}
