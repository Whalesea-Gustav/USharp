#pragma once
#include "BmwGameThreadDeferQueue.h"
#include "HAL/CriticalSection.h"

struct FBmwDeferPolicy
{
	using Object = UObject;
	using Function = UFunction;
	enum { CapacityIncrement = 0x1000000 };
	FCriticalSection CriticalSection;
	bool IsGameThread() const { return IsInGameThread(); }
	void Enter() { CriticalSection.Lock(); }
	void Leave() { CriticalSection.Unlock(); }
	void* Allocate(std::size_t Bytes) { return FMemory::Malloc(Bytes, 16); }
	void Free(void* Buffer) { FMemory::Free(Buffer); }
	void Copy(void* To, const void* From, std::size_t Bytes)
	{
		if (Bytes) FMemory::Memcpy(To, From, Bytes);
	}
	std::uint16_t ParamsSize(UFunction* Function) { return Function->ParmsSize; }
	void Dispatch(UObject* Object, UFunction* Function, void* Parameters)
	{
		Object->ProcessEvent(Function, Parameters);
	}
	void DestroyParameters(UFunction* Function, void* Parameters)
	{
		for (TFieldIterator<UProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			if (!It->HasAnyPropertyFlags(CPF_NoDestructor))
				It->DestroyValue_InContainer(Parameters);
		}
	}
};

inline TBmwGameThreadDeferQueue<FBmwDeferPolicy>& GetBmwGameThreadDeferQueue()
{
	static FBmwDeferPolicy Policy;
	static TBmwGameThreadDeferQueue<FBmwDeferPolicy> Queue(Policy);
	return Queue;
}
