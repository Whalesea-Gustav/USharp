#pragma once
#include <cstdint>
#include <cstring>

// Count, capacity and byte offsets intentionally follow the retained BMW instructions.
template <typename Policy>
class TBmwGameThreadDeferQueue
{
public:
	using Object = typename Policy::Object;
	using Function = typename Policy::Function;
	explicit TBmwGameThreadDeferQueue(Policy& InPolicy) : Host(InPolicy) {}
	~TBmwGameThreadDeferQueue() { Host.Free(Data); }
	TBmwGameThreadDeferQueue(const TBmwGameThreadDeferQueue&) = delete;
	TBmwGameThreadDeferQueue& operator=(const TBmwGameThreadDeferQueue&) = delete;

	static std::uint16_t Stride(std::uint16_t Size)
	{
		return static_cast<std::uint16_t>((((Size >> 3) + 2) & 0xfffe) + 2);
	}

	int ProcessEvent(Object* Target, Function* Method, void* Parameters)
	{
		if (Host.IsGameThread())
		{
			Host.Dispatch(Target, Method, Parameters);
			return 0;
		}
		Host.Enter();
		const std::uint16_t Size = Host.ParamsSize(Method);
		const std::uint16_t Step = Stride(Size);
		if (Capacity < Count + Step)
		{
			Capacity += Policy::CapacityIncrement;
			auto* Next = static_cast<std::uint8_t*>(Host.Allocate(Capacity));
			Host.Copy(Next, Data, Count);
			Host.Free(Data);
			Data = Next;
		}
		auto* Record = Data + static_cast<std::size_t>(Count) * 8;
		std::memcpy(Record, &Target, 8);
		std::memcpy(Record + 8, &Method, 8);
		Host.Copy(Record + 16, Parameters, Size);
		Count += Step;
		Host.Leave();
		return 1;
	}

	void Flush()
	{
		Host.Enter();
		std::int32_t Cursor = 0;
		while (Cursor < Count)
		{
			auto* Record = Data + static_cast<std::size_t>(Cursor) * 8;
			Object* Target;
			Function* Method;
			std::memcpy(&Target, Record, 8);
			std::memcpy(&Method, Record + 8, 8);
			void* Parameters = Record + 16;
			Host.Dispatch(Target, Method, Parameters);
			Host.DestroyParameters(Method, Parameters);
			Cursor += Stride(Host.ParamsSize(Method));
		}
		Count = 0;
		Host.Leave();
	}

	std::int32_t CountForDiagnostics() const { return Count; }

private:
	static_assert(sizeof(Object*) == 8 && sizeof(Function*) == 8, "BMW queue evidence is x64");
	Policy& Host;
	std::uint8_t* Data = nullptr;
	std::int32_t Count = 0;
	std::int32_t Capacity = 0;
};
