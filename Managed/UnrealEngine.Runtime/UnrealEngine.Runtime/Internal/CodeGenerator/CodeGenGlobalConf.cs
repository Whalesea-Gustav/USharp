using System.Collections.Generic;

namespace UnrealEngine.Runtime
{
    public static class CodeGenGlobalConf
    {
        public static HashSet<string> ClassSupportDeferGameThread = new HashSet<string>
        {
            "/Script/UnrealExtent.GSE_NiagaraFuncLib", "/Script/Engine.FXSystemComponent",
            "/Script/Engine.ParticleSystemComponent", "/Script/Niagara.NiagaraComponent",
            "/Script/Niagara.NiagaraFunctionLibrary", "/Script/Niagara.NiagaraDataInterfaceArrayFunctionLibrary",
            "/Script/Engine.MaterialInstanceDynamic", "/Script/b1.BGUFunctionLibAK",
            "/Script/Engine.GameplayStatics", "/Script/Engine.KismetMaterialLibrary",
            "/Script/Engine.KismetSystemLibrary", "/Script/Engine.KismetRenderingLibrary"
        };
    }
}
