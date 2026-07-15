param(
	[int]$Port = 7913,
	[int]$ResX = 900,
	[int]$ResY = 506,
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\InteractivePlaytest",
	[switch]$ValidateOnly
)

$ErrorActionPreference = "Stop"

$global:LASTEXITCODE = 0
& (Join-Path $PSScriptRoot "TestManualAcceptanceDefinition.ps1")
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$StartScript = Join-Path $PSScriptRoot "StartPackagedLocalSmoke.ps1"
$global:LASTEXITCODE = 0
& $StartScript `
	-ClientCount 2 `
	-Port $Port `
	-Address "127.0.0.1:$Port" `
	-ResX $ResX `
	-ResY $ResY `
	-OutputRoot $OutputRoot `
	-SessionLabel "Crysta/Void owner-observer acceptance" `
	-VerifyCandidate `
	-ValidateOnly:$ValidateOnly

if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}
if ($ValidateOnly) {
	Write-Host "Crysta/Void acceptance launcher validation: PASS"
	Write-Host "No session record or process was created."
	$global:LASTEXITCODE = 0
	return
}

$RunDirectory = (Get-Content -LiteralPath (Join-Path $OutputRoot "LatestSession.txt") -Raw).Trim()
$InstructionsPath = Join-Path $RunDirectory "CrystaVoidInstructions.txt"
$AcceptanceRecordPath = Join-Path $RunDirectory "CrystaVoid-ManualAcceptance.md"

& (Join-Path $PSScriptRoot "NewManualAcceptanceRecord.ps1") `
	-SessionDirectory $RunDirectory `
	-Batch CrystaVoid

$Instructions = @(
	"Breachborne Crysta/Void manual acceptance",
	"Session: $RunDirectory",
	"",
	"Lobby roles:",
	"  Client 1: Team 0 / Slot 0 / Crysta (hunter 5)",
	"  Client 2: Team 1 / Slot 0 / Void (hunter 6)",
	"",
	"Test Crysta R at close and far convergence, and hold/release RMB while changing aim.",
	"Build Void empowered with three confirmed hits before empowered RMB/Q/R comparisons.",
	"Use a wall for charged LMB and nearby chest/unmarked props for Shift eligibility checks.",
	"The frozen package has no placed UBBVoidSwappableComponent prop fixture; record that marked-prop",
	"visual subcase as SETUP BLOCKED, not PASS. Deterministic category teleport coverage is separate.",
	"",
	"Crysta checks: LMB/mark, RMB curve/return, Shift charges, Q, X/V R.",
	"Void checks: LMB, passive, RMB cone, Shift endpoints/swap, Q, R.",
	"",
	"Stop and review:",
	"  .\Scripts\Playtest\StopPackagedLocalSmoke.ps1",
	"  Record results: $AcceptanceRecordPath"
)
$Instructions | Set-Content -LiteralPath $InstructionsPath -Encoding UTF8

Write-Host ""
Write-Host "Crysta/Void acceptance roles:"
Write-Host "  Client 1: Crysta, Team 0 / Slot 0"
Write-Host "  Client 2: Void,   Team 1 / Slot 0"
Write-Host "  Instructions: $InstructionsPath"
Write-Host "  Result record: $AcceptanceRecordPath"
Write-Host "Marked-prop visual swap is setup-blocked in this frozen package; do not infer PASS."
Write-Host "Do not close clients individually; use StopPackagedLocalSmoke.ps1 so logs are reviewed."

exit 0
