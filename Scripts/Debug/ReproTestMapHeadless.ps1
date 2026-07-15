param(
    [string]$ProjectPath = "C:\Unreal Projects\Breachborne\Breachborne.uproject",
    [string]$EngineRoot = "C:\UnrealEngine-5.7.4-release",
    [string]$Map = "/Game/Maps/TestMap",
    [switch]$UseBlueprintGameMode
)

$editorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if (-not (Test-Path -LiteralPath $editorCmd)) {
    throw "UnrealEditor-Cmd.exe not found: $editorCmd"
}

if (-not (Test-Path -LiteralPath $ProjectPath)) {
    throw "Project not found: $ProjectPath"
}

$repoRoot = Split-Path $ProjectPath
$crashRoot = Join-Path $repoRoot "Saved\Crashes"
$before = @{}
if (Test-Path -LiteralPath $crashRoot) {
    Get-ChildItem -LiteralPath $crashRoot -Directory | ForEach-Object { $before[$_.FullName] = $true }
}

$mapArg = $Map
if ($UseBlueprintGameMode) {
    $mapArg = "${Map}?game=/Game/BP_BreachborneGameMode.BP_BreachborneGameMode_C"
}

$arguments = @(
    "`"$ProjectPath`"",
    "`"$mapArg`"",
    "-game",
    "-log",
    "-nop4",
    "-unattended",
    "-NoSplash",
    "-nullrhi",
    "-nosound",
    "-trace=cpu,frame,bookmark,loadtime,file",
    "-statnamedevents",
    "-ExecCmds=quit"
)

Write-Host "Running: $editorCmd $($arguments -join ' ')"
$process = Start-Process -FilePath $editorCmd -ArgumentList $arguments -WorkingDirectory (Split-Path $editorCmd) -Wait -PassThru
Write-Host "Exit code: $($process.ExitCode)"

if (Test-Path -LiteralPath $crashRoot) {
    $newCrash = Get-ChildItem -LiteralPath $crashRoot -Directory |
        Where-Object { -not $before.ContainsKey($_.FullName) } |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1

    if ($newCrash) {
        Write-Host "New crash report: $($newCrash.FullName)"
        $context = Get-ChildItem -LiteralPath $newCrash.FullName -Filter "CrashContext.runtime-xml" -File -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($context) {
            Write-Host "Crash context: $($context.FullName)"
        }
    }
}

$log = Join-Path $repoRoot "Saved\Logs\Breachborne.log"
if (Test-Path -LiteralPath $log) {
    Write-Host "Latest log tail:"
    Get-Content -LiteralPath $log -Tail 80
}

exit $process.ExitCode
