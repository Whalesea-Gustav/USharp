#include <cstdlib>
#include <iostream>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>
#include "../../Source/USharp/Private/BmwGameThreadDeferQueue.h"

struct Target { int Id; };
struct Method { std::uint16_t Size; bool NeedsDestructor; };
struct Host
{
	using Object = Target;
	using Function = Method;
	enum { CapacityIncrement = 64 };
	bool Game = false;
	int Depth = 0;
	std::recursive_mutex Mutex;
	std::vector<std::string> Events;
	std::vector<std::size_t> CopyLengths;
	void* Latest = nullptr;
	std::function<void(int)> OnCall;
	bool IsGameThread() const { return Game; }
	void Enter() { Mutex.lock(); ++Depth; }
	void Leave() { --Depth; Mutex.unlock(); }
	void* Allocate(std::size_t Bytes)
	{
		// Padded diagnostic backing prevents UB while exposing the original capacity-unit mismatch.
		Latest = std::calloc(8, Bytes);
		return Latest;
	}
	void Free(void* Buffer) { std::free(Buffer); }
	void Copy(void* To, const void* From, std::size_t Bytes)
	{
		CopyLengths.push_back(Bytes);
		if (Bytes) std::memcpy(To, From, Bytes);
	}
	std::uint16_t ParamsSize(Method* Function) { return Function->Size; }
	void Dispatch(Target*, Method*, void* Params)
	{
		int Value;
		std::memcpy(&Value, Params, sizeof(Value));
		Events.push_back("call:" + std::to_string(Value));
		if (OnCall) OnCall(Value);
	}
	void DestroyParameters(Method* Function, void* Params)
	{
		if (Depth != 1) throw std::runtime_error("flush did not retain lock");
		if (Function->NeedsDestructor)
		{
			int Value;
			std::memcpy(&Value, Params, sizeof(Value));
			Events.push_back("destroy:" + std::to_string(Value));
		}
	}
};

int main()
{
	int Checks = 0;
	auto Require = [&](bool Value)
	{
		if (!Value) throw std::runtime_error("defer contract failed");
		++Checks;
	};
	try
	{
		using Queue = TBmwGameThreadDeferQueue<Host>;
		Require(Queue::Stride(0) == 4 && Queue::Stride(4) == 4 && Queue::Stride(16) == 6 &&
			Queue::Stride(31) == 6 && Queue::Stride(32) == 8);
		Host Policy;
		Queue Pending(Policy);
		Target Object{1};
		Method Function{4, true};
		int First = 10, Second = 20;
		Policy.Game = true;
		Require(Pending.ProcessEvent(&Object, &Function, &First) == 0);
		Require(Pending.CountForDiagnostics() == 0 && Policy.Events == std::vector<std::string>{"call:10"});
		Policy.Events.clear();
		Policy.Game = false;
		Require(Pending.ProcessEvent(&Object, &Function, &First) == 1);
		Require(Pending.ProcessEvent(&Object, &Function, &Second) == 1);
		First = 999;
		Require(Policy.Events.empty() && Pending.CountForDiagnostics() == 8);
		Pending.Flush();
		Require(Policy.Events == std::vector<std::string>{"call:10", "destroy:10", "call:20", "destroy:20"});
		Require(Policy.Depth == 0 && Pending.CountForDiagnostics() == 0);
		Pending.Flush();
		Require(Policy.Events.size() == 4);
		Policy.Events.clear();
		Policy.OnCall = [&](int Value)
		{
			if (Value == 30)
			{
				int Tail = 40;
				Require(Pending.ProcessEvent(&Object, &Function, &Tail) == 1);
			}
		};
		int Reentrant = 30;
		Pending.ProcessEvent(&Object, &Function, &Reentrant);
		Pending.Flush();
		Require(Policy.Events == std::vector<std::string>{"call:30", "destroy:30", "call:40", "destroy:40"});
		Policy.OnCall = nullptr;
		Policy.Events.clear();
		Method Plain{4, false};
		Pending.ProcessEvent(&Object, &Plain, &Second);
		Pending.Flush();
		Require(Policy.Events == std::vector<std::string>{"call:20"});

		Host Growth;
		Queue Relocated(Growth);
		for (int Index = 0; Index < 16; ++Index)
			Relocated.ProcessEvent(&Object, &Plain, &Second);
		Require(Relocated.CountForDiagnostics() == 64);
		Relocated.ProcessEvent(&Object, &Plain, &Second);
		Require(Growth.CopyLengths[Growth.CopyLengths.size() - 2] == 64);
		Method* LostMethod = reinterpret_cast<Method*>(1);
		std::memcpy(&LostMethod, static_cast<std::uint8_t*>(Growth.Latest) + 8 * 8 + 8, 8);
		Require(LostMethod == nullptr);
		std::cout << "PASS checks=" << Checks << "; legacy short relocation copy reproduced with padded diagnostic memory\n";
		return 0;
	}
	catch (const std::exception& Error)
	{
		std::cerr << Error.what() << "\n";
		return 1;
	}
}
