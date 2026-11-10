#pragma once

template <typename ClassType, typename InitializerType>
void DispatchSharpManagedConstructor(ClassType* Class, const InitializerType& Initializer)
{
	Class->ManagedConstructor(Class, Initializer);
}
