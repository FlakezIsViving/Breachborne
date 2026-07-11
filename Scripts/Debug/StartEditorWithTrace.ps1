param(
    [string]$ProjectPath = "C:\Unreal Projects\Breachborne\Breachborne.uproject",
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.7",
    [switch]$Wait
)

$editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
if (-not (Test-Path -LiteralPath $editor)) {
    throw "UnrealEditor.exe not found: $editor"
}

if (-not (Test-Path -LiteralPath $ProjectPath)) {
    throw "Project not found: $ProjectPath"
}

$arguments = @(
    "`"$ProjectPath`"",
    "-log",
    "-trace=cpu,frame,bookmark,loadtime,file",
    "-statnamedevents"
)

$process = Start-Process -FilePath $editor -ArgumentList $arguments -WorkingDirectory (Split-Path $editor) -PassThru
Write-Host "Started UnrealEditor.exe with PID $($process.Id)"
Write-Host "Attach debugger to UnrealEditor.exe, then press Play in PIE."

if ($Wait) {
    Wait-Process -Id $process.Id
}
