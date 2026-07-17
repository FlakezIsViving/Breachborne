param(
	[Parameter(Mandatory = $true)]
	[ValidatePattern("^Breachborne\.")]
	[string]$TestFilter,
	[int]$ExpectedTestCount = 0,
	[string]$ProjectPath = "$PSScriptRoot\..\..\Breachborne.uproject",
	[string]$EngineRoot = "C:\UnrealEngine-5.7.4-release",
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\Automation",
	[switch]$DryRun
)

$ErrorActionPreference = "Stop"

$ResolvedProject = (Resolve-Path -LiteralPath $ProjectPath).ProviderPath
$ResolvedEngine = (Resolve-Path -LiteralPath $EngineRoot).ProviderPath
$EditorCmd = Join-Path $ResolvedEngine "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if (-not (Test-Path -LiteralPath $EditorCmd -PathType Leaf)) {
	throw "UnrealEditor-Cmd.exe not found: $EditorCmd"
}

$ProjectDescriptor = Get-Content -LiteralPath $ResolvedProject -Raw | ConvertFrom-Json
$DisabledPlugins = @(
	$ProjectDescriptor.Plugins |
		Where-Object { $_.Enabled -eq $false } |
		ForEach-Object { [string]$_.Name }
)
if ($DisabledPlugins -notcontains "LudusAI") {
	$DisabledPlugins += "LudusAI"
}

$SafeFilterName = $TestFilter -replace "[^A-Za-z0-9_.-]", "_"
$ResolvedOutputRoot = [IO.Path]::GetFullPath($OutputRoot)
$RunDirectory = Join-Path $ResolvedOutputRoot ("{0}-{1}" -f (Get-Date -Format "yyyyMMdd-HHmmss"), $SafeFilterName)
$LogPath = Join-Path $RunDirectory "Automation.log"
$SummaryPath = Join-Path $RunDirectory "Summary.txt"

$Arguments = @(
	$ResolvedProject,
	"-DisablePlugins=$($DisabledPlugins -join ',')",
	"-unattended",
	"-nop4",
	"-nosplash",
	"-nullrhi",
	"-nosound",
	"-abslog=$LogPath",
	"-ExecCmds=Automation RunTests $TestFilter; Quit",
	"-TestExit=Automation Test Queue Empty"
)

Write-Host "Automation filter: $TestFilter"
Write-Host "Expected tests: $ExpectedTestCount"
Write-Host "Command: $EditorCmd $($Arguments -join ' ')"
if ($DryRun) {
	return
}

$RunningEditor = Get-Process -Name "UnrealEditor*" -ErrorAction SilentlyContinue
if ($RunningEditor) {
	$ProcessList = ($RunningEditor | ForEach-Object { $_.Id }) -join ", "
	throw "Close Unreal Editor before running automation. Running PID(s): $ProcessList"
}

New-Item -ItemType Directory -Force -Path $RunDirectory | Out-Null
& $EditorCmd @Arguments
$ProcessExitCode = $LASTEXITCODE
if (-not (Test-Path -LiteralPath $LogPath -PathType Leaf)) {
	throw "Automation produced no log: $LogPath"
}

$LogText = Get-Content -LiteralPath $LogPath -Raw
$EscapedFilter = [regex]::Escape($TestFilter)
$StartedCount = ([regex]::Matches($LogText, "Test Started\..*Path=\{$EscapedFilter[^}]*\}")).Count
$PassedCount = ([regex]::Matches($LogText, "Test Completed\. Result=\{Success\}.*Path=\{$EscapedFilter[^}]*\}")).Count
$FailedCount = ([regex]::Matches($LogText, "Test Completed\. Result=\{(Fail|Error|NotRun)\}.*Path=\{$EscapedFilter[^}]*\}")).Count
$CriticalCount = ([regex]::Matches($LogText,
	"Fatal error|Unhandled Exception|Assertion failed|Ensure condition failed",
	[Text.RegularExpressions.RegexOptions]::IgnoreCase)).Count

$Failures = @()
if ($ProcessExitCode -ne 0) {
	$Failures += "Editor command exited with code $ProcessExitCode."
}
if ($ExpectedTestCount -gt 0 -and $StartedCount -ne $ExpectedTestCount) {
	$Failures += "Expected $ExpectedTestCount tests but observed $StartedCount starts."
}
if ($StartedCount -eq 0) {
	$Failures += "No matching tests started."
}
if ($PassedCount -ne $StartedCount) {
	$Failures += "Only $PassedCount of $StartedCount started tests passed."
}
if ($FailedCount -gt 0) {
	$Failures += "$FailedCount tests reported a failing result."
}
if ($CriticalCount -gt 0) {
	$Failures += "$CriticalCount critical log findings were detected."
}

$Result = if ($Failures.Count -eq 0) { "PASS" } else { "FAIL" }
$Summary = @(
	"Breachborne editor automation: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Filter: $TestFilter",
	"Expected tests: $ExpectedTestCount",
	"Started: $StartedCount",
	"Passed: $PassedCount",
	"Failed results: $FailedCount",
	"Critical findings: $CriticalCount",
	"Process exit code: $ProcessExitCode",
	"Log: $LogPath"
)
if ($Failures.Count -gt 0) {
	$Summary += "Failures:"
	$Summary += $Failures | ForEach-Object { "- $_" }
}
$Summary | Set-Content -LiteralPath $SummaryPath -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Failures.Count -gt 0) {
	throw "Editor automation failed for $TestFilter. See $SummaryPath"
}
