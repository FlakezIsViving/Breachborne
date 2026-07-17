param(
    [string]$EngineRoot = "C:\UnrealEngine-5.7.4-release"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$ProjectFile = Join-Path $ProjectRoot "Breachborne.uproject"
$EditorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$BuildBat = Join-Path $EngineRoot "Engine\Build\BatchFiles\Build.bat"
$PythonScript = Join-Path $PSScriptRoot "CreateGhostQGameplayCues.py"
$LogsRoot = Join-Path $ProjectRoot "Saved\Logs"

if (Get-Process -Name "UnrealEditor*" -ErrorAction SilentlyContinue) {
    throw "Close Unreal Editor before building and creating the Ghost Q GameplayCue assets."
}

foreach ($RequiredPath in @($ProjectFile, $EditorCmd, $BuildBat, $PythonScript)) {
    if (-not (Test-Path -LiteralPath $RequiredPath)) {
        throw "Required path does not exist: $RequiredPath"
    }
}

$BuildArguments = @(
    "BreachborneEditor",
    "Win64",
    "Development",
    "-Project=$ProjectFile",
    "-WaitMutex",
    "-NoHotReloadFromIDE"
)
& $BuildBat @BuildArguments
if ($LASTEXITCODE -ne 0) {
    throw "BreachborneEditor build failed with exit code $LASTEXITCODE."
}

$EditorArguments = @(
    $ProjectFile,
    "-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities",
    "-DisablePlugins=LudusAI",
    "-Unattended",
    "-NoP4",
    "-NoSplash",
    "-NullRHI",
    "-NoSound",
    "-Log",
    "-ExecutePythonScript=$PythonScript"
)
& $EditorCmd @EditorArguments
if ($LASTEXITCODE -ne 0) {
    throw "Ghost Q GameplayCue creation failed with exit code $LASTEXITCODE."
}

$LatestLog = Get-ChildItem -LiteralPath $LogsRoot -Filter "*.log" -File |
    Sort-Object LastWriteTime -Descending |
    Select-Object -First 1
if (-not $LatestLog -or -not (Select-String -LiteralPath $LatestLog.FullName -Pattern "BB_GHOST_Q_VFX\|READY" -Quiet)) {
    throw "Ghost Q GameplayCue creation did not emit BB_GHOST_Q_VFX|READY in the latest project log."
}

Write-Host "Ghost Q GameplayCue assets were created and validated."
