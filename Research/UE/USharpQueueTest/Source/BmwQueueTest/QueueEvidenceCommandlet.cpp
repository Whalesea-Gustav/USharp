#include "QueueEvidenceCommandlet.h"
#include "BmwGameThreadDeferQueue.h"
#include "Modules/ModuleManager.h"
#include "HAL/CriticalSection.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include <thread>

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, BmwQueueTest, "USharpQueueTest");

namespace
{
	struct FEngineQueuePolicy
	{
		using Object = UObject;
		using Function = UFunction;
		enum { CapacityIncrement = 0x1000000 };
		FCriticalSection Lock;
		int32 Destroyed = 0;
		int32 CopiedBytes = 0;
		bool IsGameThread() const { return IsInGameThread(); }
		void Enter() { Lock.Lock(); }
		void Leave() { Lock.Unlock(); }
		void* Allocate(std::size_t Bytes) { return FMemory::Malloc(Bytes, 16); }
		void Free(void* Buffer) { FMemory::Free(Buffer); }
		void Copy(void* To, const void* From, std::size_t Bytes)
		{
			CopiedBytes += static_cast<int32>(Bytes);
			if (Bytes) FMemory::Memcpy(To, From, Bytes);
		}
		std::uint16_t ParamsSize(UFunction* Function) { return Function->ParmsSize; }
		void Dispatch(UObject* Object, UFunction* Function, void* Parameters)
		{
			check(IsInGameThread());
			Object->ProcessEvent(Function, Parameters);
		}
		void DestroyParameters(UFunction* Function, void* Parameters)
		{
			for (TFieldIterator<FProperty> It(Function); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
			{
				if (!It->HasAnyPropertyFlags(CPF_NoDestructor))
				{
					++Destroyed;
					It->DestroyValue_InContainer(Parameters);
				}
			}
		}
	};
}

void UQueueEvidenceReceiver::Record(int32 Value) { Events.Add(FString::Printf(TEXT("int:%d"), Value)); }
void UQueueEvidenceReceiver::RecordText(const FString& Value) { Events.Add(TEXT("text:") + Value); }

UQueueEvidenceCommandlet::UQueueEvidenceCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UQueueEvidenceCommandlet::Main(const FString& Params)
{
	FEngineQueuePolicy Host;
	TBmwGameThreadDeferQueue<FEngineQueuePolicy> Queue(Host);
	TStrongObjectPtr<UQueueEvidenceReceiver> Receiver(NewObject<UQueueEvidenceReceiver>());
	UFunction* IntFunction = Receiver->FindFunctionChecked(TEXT("Record"));
	UFunction* TextFunction = Receiver->FindFunctionChecked(TEXT("RecordText"));
	FIntProperty* IntProperty = FindFProperty<FIntProperty>(IntFunction, TEXT("Value"));
	FStrProperty* TextProperty = FindFProperty<FStrProperty>(TextFunction, TEXT("Value"));
	check(IntProperty && TextProperty);
	int32 Checks = 0;
	auto Require = [&Checks](bool Condition) { check(Condition); ++Checks; };
	void* IntArgs = FMemory::Malloc(IntFunction->ParmsSize, 16);
	IntFunction->InitializeStruct(IntArgs);
	IntProperty->SetPropertyValue_InContainer(IntArgs, 10);
	Require(Queue.ProcessEvent(Receiver.Get(), IntFunction, IntArgs) == 0);
	Require(Receiver->Events.Num() == 1 && Host.Destroyed == 0 && Queue.CountForDiagnostics() == 0);
	IntProperty->SetPropertyValue_InContainer(IntArgs, 20);
	int Deferred = -1;
	std::thread IntWorker([&] { Deferred = Queue.ProcessEvent(Receiver.Get(), IntFunction, IntArgs); });
	IntWorker.join();
	Require(Deferred == 1 && Receiver->Events.Num() == 1);
	IntProperty->SetPropertyValue_InContainer(IntArgs, 999);
	IntFunction->DestroyStruct(IntArgs);
	FMemory::Free(IntArgs);

	void* TextArgs = FMemory::Malloc(TextFunction->ParmsSize, 16);
	TextFunction->InitializeStruct(TextArgs);
	TextProperty->SetPropertyValue_InContainer(TextArgs, TEXT("owned-by-queue"));
	std::thread TextWorker([&] { Deferred = Queue.ProcessEvent(Receiver.Get(), TextFunction, TextArgs); });
	TextWorker.join();
	Require(Deferred == 1 && Host.Destroyed == 0);
	// The sender relinquishes the shallow-copied FString, as generated C# does on defer.
	FMemory::Memzero(TextArgs, TextFunction->ParmsSize);
	FMemory::Free(TextArgs);
	Queue.Flush();
	Require(Receiver->Events.Num() == 3 && Receiver->Events[1] == TEXT("int:20") &&
		Receiver->Events[2] == TEXT("text:owned-by-queue"));
	Require(Host.Destroyed == 1 && Queue.CountForDiagnostics() == 0);
	Queue.Flush();
	Require(Receiver->Events.Num() == 3 && Host.Destroyed == 1);
	const FString Json = FString::Printf(
		TEXT("{\"success\":true,\"checks\":%d,\"events\":%d,\"destroyed\":%d,\"copied_bytes\":%d,\"scope\":\"UE5 ProcessEvent adapter and shared queue core\"}"),
		Checks, Receiver->Events.Num(), Host.Destroyed, Host.CopiedBytes);
	const bool Saved = FFileHelper::SaveStringToFile(Json, *(FPaths::ProjectSavedDir() / TEXT("queue-evidence.json")));
	UE_LOG(LogTemp, Display, TEXT("BMW_QUEUE_TEST %s"), *Json);
	return Saved ? 0 : 1;
}
