$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/..").Path
$artifacts = "$root/ResearchArtifacts/GameThreadDefer"
New-Item -ItemType Directory $artifacts -Force | Out-Null
& "$PSScriptRoot/run-validation.ps1"
$command = 'call "D:\VisualStudio22\VC\Auxiliary\Build\vcvars64.bat" >nul && cl /nologo /EHsc /W4 /std:c++17 "' + $root + '\Research\Tests\defer_contract.cpp" /Fe:"' + $artifacts + '\defer_contract.exe" /Fo:"' + $artifacts + '\defer_contract.obj"'
& cmd.exe /d /c $command
if ($LASTEXITCODE) { throw 'Defer contract compile failed' }
& "$artifacts/defer_contract.exe" | Tee-Object "$artifacts/native-defer-result.txt"
if ($LASTEXITCODE) { throw 'Defer contract failed' }
python "$PSScriptRoot/verify-bmw-defer-evidence.py"
if ($LASTEXITCODE) { throw 'Original evidence verification failed' }
$project = "$PSScriptRoot/UE/USharpQueueTest"
$busy = Get-CimInstance Win32_Process | Where-Object {
    $_.Name -match 'UnrealEditor|AutomationTool' -or
    ($_.Name -match 'dotnet' -and $_.CommandLine -match 'UnrealBuildTool')
}
if ($busy) { throw 'UE or UBT active; leave existing work running' }
& 'G:/Projects/UnrealProjects/UnrealEngine/Engine/Build/BatchFiles/Build.bat' USharpQueueTestEditor Win64 DebugGame "-Project=$project/USharpQueueTest.uproject" -UsePrecompiled -gather -NoEngineChanges -WaitMutex "-Log=$project/Saved/queue-repeat-build.log"
if ($LASTEXITCODE) { throw 'UE test module build failed' }
$busy = Get-CimInstance Win32_Process | Where-Object {
    $_.Name -match 'UnrealEditor|AutomationTool' -or
    ($_.Name -match 'dotnet' -and $_.CommandLine -match 'UnrealBuildTool')
}
if ($busy) { throw 'UE or UBT active before test' }
$arguments = @("$project/USharpQueueTest.uproject", '-run=QueueEvidence', '-unattended', '-NullRHI',
    '-nosound', '-nop4', '-NoLiveCoding', '-UTF8Output', "-abslog=$project/Saved/queue-repeat-editor.log")
$process = Start-Process 'G:/Projects/UnrealProjects/UnrealEngine/Engine/Binaries/Win64/UnrealEditor-Win64-DebugGame-Cmd.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru -Wait
if ($process.ExitCode -ne 0) { throw 'UE queue commandlet failed' }
$report = Get-Content -Raw "$project/Saved/queue-evidence.json" | ConvertFrom-Json
if (!$report.success -or $report.events -ne 3 -or $report.destroyed -ne 1) { throw 'UE queue report failed' }
$report
