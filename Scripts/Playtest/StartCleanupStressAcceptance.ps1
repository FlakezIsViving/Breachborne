param(
	[int]$Port = 7915,
	[int]$ResX = 720,
	[int]$ResY = 405,
	[int]$PktLag = 0,
	[int]$PktLoss = 0,
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\InteractivePlaytest",
	[switch]$ValidateOnly
)

$ErrorActionPreference = "Stop"

if ($PktLag -lt 0) {
	throw "PktLag cannot be negative."
}
if ($PktLoss -lt 0 -or $PktLoss -gt 100) {
	throw "PktLoss must be between 0 and 100."
}

$global:LASTEXITCODE = 0
& (Join-Path $PSScriptRoot "TestManualAcceptanceDefinition.ps1")
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$ExtraArgs = @()
if ($PktLag -gt 0 -or $PktLoss -gt 0) {
	$ExtraArgs = @("-ExecCmds=`"Net PktLag=$PktLag,Net PktLoss=$PktLoss`"")
}

$StartScript = Join-Path $PSScriptRoot "StartPackagedLocalSmoke.ps1"
$global:LASTEXITCODE = 0
& $StartScript `
	-ClientCount 4 `
	-Port $Port `
	-Address "127.0.0.1:$Port" `
	-ResX $ResX `
	-ResY $ResY `
	-OutputRoot $OutputRoot `
	-SessionLabel "Cleanup/stress acceptance (PktLag=$PktLag PktLoss=$PktLoss)" `
	-ServerExtraArgs $ExtraArgs `
	-ClientExtraArgs $ExtraArgs `
	-VerifyCandidate:(-not $ValidateOnly) `
	-ValidateOnly:$ValidateOnly

if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}
if ($ValidateOnly) {
	Write-Host "Cleanup/stress acceptance launcher validation: PASS"
	Write-Host "  PktLag:  $PktLag ms"
	Write-Host "  PktLoss: $PktLoss percent"
	Write-Host "No session record or process was created."
	$global:LASTEXITCODE = 0
	return
}

$RunDirectory = (Get-Content -LiteralPath (Join-Path $OutputRoot "LatestSession.txt") -Raw).Trim()
$InstructionsPath = Join-Path $RunDirectory "CleanupStressInstructions.txt"
$AcceptanceRecordPath = Join-Path $RunDirectory "CleanupStress-ManualAcceptance.md"

& (Join-Path $PSScriptRoot "NewManualAcceptanceRecord.ps1") `
	-SessionDirectory $RunDirectory `
	-Batch CleanupStress

$ModeDescription = if ($PktLag -eq 0 -and $PktLoss -eq 0) {
	"NORMAL"
} else {
	"IMPAIRED: PktLag=$PktLag ms / PktLoss=$PktLoss percent on server and all clients"
}
$Instructions = @(
	"Breachborne cleanup/stress manual acceptance",
	"Session: $RunDirectory",
	"Mode: $ModeDescription",
	"",
	"Use two normal match cycles to cover all six casters:",
	"  Game 1: Ghost and Eluna versus Hudson and Crysta",
	"  Game 2: Void and Kingpin plus helpers selected as needed",
	"Kill each caster during the persistent abilities listed in CS-01.",
	"After each death, inspect owner and observer for stale meshes, cues, timers, movement, hooks,",
	"empowered state, and world actors before continuing.",
	"",
	"Grappling Hook setup from the owning client's console:",
	"  DebugGivePower GrapplingHook 1",
	"Test wall hit, actor hit, maximum range, timeout, pull-once behavior, and observer chain cleanup.",
	"",
	"Effects-quality pass on each visible client:",
	"  sg.EffectsQuality 0",
	"  sg.EffectsQuality 1",
	"Repeat representative beams, zones, sustained fire, and wisp presentation at both settings.",
	"",
	"For CS-03 run a separate session with:",
	"  .\Scripts\Playtest\StartCleanupStressAcceptance.ps1 -PktLag 100 -PktLoss 2",
	"In that session, hold Hudson LMB for ten seconds and overlap one persistent ground zone.",
	"The server and every client log must confirm PktLag/PktLoss before CS-03 can pass.",
	"Stopping the impaired session restores both values because they are process-local.",
	"",
	"Do not infer cleanup or readability from automation logs alone.",
	"",
	"Stop and review:",
	"  .\Scripts\Playtest\StopPackagedLocalSmoke.ps1",
	"  Record results: $AcceptanceRecordPath"
)
$Instructions | Set-Content -LiteralPath $InstructionsPath -Encoding UTF8

Write-Host ""
Write-Host "Cleanup/stress acceptance session ready."
Write-Host "  Mode: $ModeDescription"
Write-Host "  Instructions: $InstructionsPath"
Write-Host "  Result record: $AcceptanceRecordPath"
Write-Host "Do not close clients individually; use StopPackagedLocalSmoke.ps1 so logs are reviewed."

exit 0
