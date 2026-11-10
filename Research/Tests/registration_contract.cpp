#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "../../Source/USharp/Private/SharpFunctionRegistration.h"
#include "../../Source/USharp/Private/SharpConstructorDispatch.h"

using Callback = void (*)(int&);
void Original(int& Value) { Value += 1; }
void Managed(int& Value) { Value += 10; }
void Replacement(int& Value) { Value += 20; }
void NativeThunk(int& Value) { Value += 100; }

struct Function
{
	Callback Pointer = &Original;
	void SetNativeFunc(Callback Value) { Pointer = Value; }
};
struct Lookup { Function* Function; Callback Pointer; int Index; };
struct NativeEntry { std::string Name; Callback Pointer; };
struct LookupMap
{
	std::unordered_map<std::string, Lookup> Entries;
	int Num() const { return static_cast<int>(Entries.size()); }
	Lookup* Find(const std::string& Name)
	{
		auto It = Entries.find(Name);
		return It == Entries.end() ? nullptr : &It->second;
	}
};
struct NativeTable : std::vector<NativeEntry>
{
	int Num() const { return static_cast<int>(size()); }
	bool IsValidIndex(int Index) const { return Index >= 0 && Index < Num(); }
};
struct Class
{
	LookupMap FuncMapEx;
	NativeTable NativeFunctionLookupTable;
};
struct ConstructorClass
{
	void (*ManagedConstructor)(ConstructorClass*, const int&);
	int Observed = 0;
};
void Construct(ConstructorClass* Owner, const int& Initializer) { Owner->Observed = Initializer; }

int main()
{
	Function Target;
	Class Owner;
	Owner.FuncMapEx.Entries.emplace("Update", Lookup{&Target, &Original, 0});
	Owner.NativeFunctionLookupTable.push_back({"Update", &Original});
	const std::string Name = "Update";
	int Checks = 0;
	auto Require = [&Checks](bool Condition)
	{
		if (!Condition) throw "registration contract failed";
		++Checks;
	};
	auto Install = [&]() { return InstallSharpFunctionInvoker(&Owner, Name, &Managed); };
	auto Unchanged = [&]()
	{
		Require(Target.Pointer == &Original &&
			Owner.FuncMapEx.Find(Name)->Pointer == &Original &&
			Owner.NativeFunctionLookupTable[0].Pointer == &Original);
	};
	try
	{
		Require(InstallSharpFunctionInvoker(&Owner, std::string("Missing"), &Managed) == nullptr);
		Unchanged();
		Owner.FuncMapEx.Find(Name)->Index = -1;
		Require(Install() == nullptr);
		Unchanged();
		Owner.FuncMapEx.Find(Name)->Index = 0;
		Owner.NativeFunctionLookupTable.push_back({"Other", &Original});
		Require(Install() == nullptr);
		Unchanged();
		Owner.NativeFunctionLookupTable.pop_back();
		Require(Install() == &Target);
		Require(Target.Pointer == &Managed && Owner.FuncMapEx.Find(Name)->Pointer == &Managed &&
			Owner.NativeFunctionLookupTable[0].Pointer == &Managed);
		int Value = 0;
		Target.Pointer(Value);
		Require(Value == 10);
		Require(InstallSharpFunctionInvoker(&Owner, Name, &Replacement) == &Target);
		Target.Pointer(Value);
		Require(Value == 30);
		Require(InstallSharpFunctionInvoker(&Owner, Name, static_cast<Callback>(nullptr)) == &Target);
		Require(Target.Pointer == nullptr && Owner.FuncMapEx.Find(Name)->Pointer == nullptr &&
			Owner.NativeFunctionLookupTable[0].Pointer == nullptr);
		ConstructorClass ConstructorOwner{&Construct};
		const int Initializer = 73;
		DispatchSharpManagedConstructor(&ConstructorOwner, Initializer);
		Require(ConstructorOwner.Observed == Initializer);
		std::cout << "PASS checks=" << Checks << "\n";
		return 0;
	}
	catch (const char* Message)
	{
		std::cerr << Message << "\n";
		return 1;
	}
}
