using System.Reflection;
using System.Runtime.Loader;
using System.Runtime.InteropServices;
using System.Collections;

internal static class Program
{
    private static int skipped;
    private static int deferResult;
    private static int directCalls;
    private static int deferredCalls;
    private static int flushCalls;
    public static void Direct(IntPtr instance, IntPtr function, IntPtr parameters) { directCalls++; }
    public static int Deferred(IntPtr instance, IntPtr function, IntPtr parameters) { deferredCalls++; return deferResult; }
    public static void Flush() { flushCalls++; }
    public static void Skip(IntPtr instance, IntPtr stack, IntPtr result, IntPtr function)
    {
        if (instance != new IntPtr(73) || function != new IntPtr(123) || stack == IntPtr.Zero || result != IntPtr.Zero)
            throw new InvalidOperationException("incorrect frame forwarding");
        skipped++;
    }
    private static int Main(string[] args)
    {
        var assembly = AssemblyLoadContext.Default.LoadFromAssemblyPath(Path.GetFullPath(args[0]));
        var native = assembly.GetType("UnrealEngine.Runtime.Native.Native_USharpClass", true)!;
        var signature = native.GetNestedType("Del_SetFunctionInvoker")!.GetMethod("Invoke")!;
        var parameters = signature.GetParameters();
        int expected = int.Parse(args[1]);
        bool valid = signature.ReturnType == typeof(IntPtr) && parameters.Length == expected &&
            parameters[0].ParameterType == typeof(IntPtr) && parameters[1].ParameterType.IsByRef &&
            (expected != 3 || parameters[2].ParameterType == typeof(IntPtr));
        if (!valid) return 1;
        if (expected == 3)
        {
            if (native.GetField("SetFunctionInvokerAddress") != null || native.GetField("GetNativeFunctionInvoker") != null ||
                native.GetField("GetFunctionInvokerAbiVersion") != null) return 2;
            var types = assembly.GetType("UnrealEngine.Runtime.ManagedUnrealTypes", true)!;
            if (types.GetProperty("UFunctionAddrToClasses") == null) return 3;
            var managedClass = types.GetNestedType("ManagedClass")!;
            var first = Activator.CreateInstance(managedClass)!;
            var second = Activator.CreateInstance(managedClass)!;
            var constructor = (Delegate)managedClass.GetProperty("LocalUSharpClassConstructor")!.GetValue(first)!;
            var constructorParameters = constructor.GetType().GetMethod("Invoke")!.GetParameters();
            if (constructorParameters.Length != 2 ||
                constructorParameters.Any(p => p.ParameterType != typeof(IntPtr))) return 8;
            var pointer = managedClass.GetProperty("FunctionInvokerAddress")!;
            var callback = managedClass.GetProperty("FunctionInvoker")!;
            var firstCallback = (Delegate)callback.GetValue(first)!;
            var secondCallback = (Delegate)callback.GetValue(second)!;
            if ((IntPtr)pointer.GetValue(first)! == IntPtr.Zero || (IntPtr)pointer.GetValue(second)! == IntPtr.Zero ||
                firstCallback.Target != null || secondCallback.Target != null ||
                firstCallback.Method != secondCallback.Method || !firstCallback.Method.IsStatic) return 4;
            Console.WriteLine("BMW surface: direct static dispatcher method; upstream-only invoker exports absent");
            var routes = (IDictionary)types.GetProperty("UFunctionAddrToClasses")!.GetValue(null)!;
            var invokers = managedClass.GetField("managedInvokersByAddress", BindingFlags.NonPublic | BindingFlags.Instance)!;
            invokers.SetValue(first, Activator.CreateInstance(invokers.FieldType));
            var frame = assembly.GetType("UnrealEngine.Runtime.FFrame", true)!;
            foreach (var offset in new[] { ("offset_CurrentNativeFunction", 0), ("offset_Code", 8), ("offset_Object", 16) })
                frame.GetField(offset.Item1, BindingFlags.NonPublic | BindingFlags.Static)!.SetValue(null, offset.Item2);
            var nativeObject = assembly.GetType("UnrealEngine.Runtime.Native.Native_UObject", true)!;
            var skipField = nativeObject.GetField("SkipFunction")!;
            skipField.SetValue(null, Delegate.CreateDelegate(skipField.FieldType, typeof(Program).GetMethod("Skip")!));
            var memory = Marshal.AllocHGlobal(32);
            try
            {
                Marshal.WriteIntPtr(memory, 0, new IntPtr(123));
                Marshal.WriteIntPtr(memory, 8, new IntPtr(1));
                Marshal.WriteIntPtr(memory, 16, new IntPtr(73));
                var dispatch = managedClass.GetMethod("InvokeFunctionStatic", BindingFlags.NonPublic | BindingFlags.Static)!;
                dispatch.Invoke(null, new object[] { new IntPtr(73), memory, IntPtr.Zero });
                if (skipped != 0) return 5;
                routes.Add(123L, first);
                dispatch.Invoke(null, new object[] { new IntPtr(73), memory, IntPtr.Zero });
                if (skipped != 1) return 6;
                routes.Clear();
                dispatch.Invoke(null, new object[] { new IntPtr(73), memory, IntPtr.Zero });
                if (skipped != 1) return 7;
                Console.WriteLine("Static routing: missing entry silent; registered owner reached; cleared route silent");
            }
            finally { Marshal.FreeHGlobal(memory); }
            foreach (var binding in new[] { ("ProcessEvent", "Direct"), ("ProcessEvent_GameThreadDefer", "Deferred"),
                ("FlushAll_GameThreadDefer", "Flush") })
            {
                var field = nativeObject.GetField(binding.Item1)!;
                field.SetValue(null, Delegate.CreateDelegate(field.FieldType, typeof(Program).GetMethod(binding.Item2)!));
            }
            var reflection = assembly.GetType("UnrealEngine.Runtime.NativeReflection", true)!;
            Type[] signatureTypes = { typeof(IntPtr), typeof(IntPtr), typeof(IntPtr), typeof(int), typeof(bool).MakeByRefType() };
            var optimized = reflection.GetMethod("InvokeFunctionOptimized", signatureTypes)!;
            object[] call = { new IntPtr(73), new IntPtr(123), IntPtr.Zero, 0, true };
            deferResult = 1;
            optimized.Invoke(null, call);
            if (!(bool)call[4] || deferredCalls != 1 || directCalls != 0) return 9;
            deferResult = 0;
            optimized.Invoke(null, call);
            if ((bool)call[4] || deferredCalls != 2 || directCalls != 0) return 10;
            optimized.Invoke(null, call);
            if (directCalls != 1 || deferredCalls != 2) return 11;
            ((Delegate)nativeObject.GetField("FlushAll_GameThreadDefer")!.GetValue(null)!).DynamicInvoke();
            if (flushCalls != 1) return 12;
            Console.WriteLine("Defer C# contract: enqueue retains ownership flag; inline clears it without duplicate dispatch; flush callable");
        }
        Console.WriteLine($"assembly={assembly.GetName().Name} parameters={parameters.Length} expected={expected} valid={valid}");
        return 0;
    }
}
