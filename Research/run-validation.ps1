$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/..").Path
$artifacts = "$root/ResearchArtifacts/FaithfulInvoker"
New-Item -ItemType Directory "$artifacts/Native" -Force | Out-Null
$project = "$root/Managed/UnrealEngine.Runtime/UnrealEngine.Runtime/UnrealEngine.Runtime.csproj"
& 'D:/VisualStudio22/MSBuild/Current/Bin/MSBuild.exe' $project /t:Build /p:Configuration=Release /p:TargetFrameworkVersion=v4.8 "/p:OutputPath=$artifacts/Managed48/" /nologo /verbosity:quiet "/flp:logfile=$artifacts/managed48-validation-build.log;verbosity=normal"
if ($LASTEXITCODE) { throw 'Managed runtime build failed' }
$command = 'call "D:\VisualStudio22\VC\Auxiliary\Build\vcvars64.bat" >nul && cl /nologo /EHsc /W4 /std:c++17 "' + $root + '\Research\Tests\registration_contract.cpp" /Fe:"' + $artifacts + '\Native\registration_contract.exe" /Fo:"' + $artifacts + '\Native\registration_contract.obj"'
& cmd.exe /d /c $command
if ($LASTEXITCODE) { throw 'Standalone native contract build failed' }
& "$artifacts/Native/registration_contract.exe" | Tee-Object "$artifacts/native-contract-result.txt"
if ($LASTEXITCODE) { throw 'Native registration contract failed' }
dotnet run --project "$PSScriptRoot/Tests/ManagedAbi/ManagedAbi.csproj" -- "$artifacts/Managed48/UnrealEngine.Runtime.dll" 3 |
    Tee-Object "$artifacts/managed-abi-result.txt"
if ($LASTEXITCODE) { throw 'Managed ABI contract failed' }
