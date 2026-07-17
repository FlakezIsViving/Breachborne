param(
	[int]$Port = 7914,
	[int]$ResX = 900,
	[int]$ResY = 460,
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\InteractivePlaytest",
	[switch]$ValidateOnly
)

$ErrorActionPreference = "Stop"

$global:LASTEXITCODE = 0
& (Join-Path $PSScriptRoot "TestManualAcceptanceDefinition.ps1")
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$global:LASTEXITCODE = 0
& (Join-Path $PSScriptRoot "StartPackagedLocalSmoke.ps1") `
	-ClientCount 3 `
	-Port $Port `
	-Address "127.0.0.1:$Port" `
	-ResX $ResX `
	-ResY $ResY `
	-OutputRoot $OutputRoot `
	-SessionLabel "Eluna RMB Shift Q repair retest" `
	-VerifyCandidate:(-not $ValidateOnly) `
	-ValidateOnly:$ValidateOnly

if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}
if ($ValidateOnly) {
	Write-Host "Eluna repair-retest launcher validation: PASS"
	Write-Host "No session record or process was created."
	$global:LASTEXITCODE = 0
	return
}

$RunDirectory = (Get-Content -LiteralPath (Join-Path $OutputRoot "LatestSession.txt") -Raw).Trim()
$InstructionsPath = Join-Path $RunDirectory "ElunaRepairRetestInstructions.txt"
$ResultPath = Join-Path $RunDirectory "ElunaRepairRetest.md"
$CandidateEvidencePath = (Resolve-Path -LiteralPath (
	Join-Path $PSScriptRoot "..\..\Builds\PlaytestCandidateVerification.txt")).ProviderPath

$Instructions = @(
	"Breachborne focused Eluna repair retest",
	"Session: $RunDirectory",
	"",
	"Lobby roles:",
	"  Client 1: bottom-left / displayed Team 1 Slot 1 / Eluna (internal team 0, hunter 3)",
	"  Client 2: top-left / displayed Team 1 Slot 2 / Hudson (internal team 0, hunter 4, allied helper)",
	"  Client 3: top-right / displayed Team 2 Slot 1 / Ghost (internal team 1, hunter 1)",
	"",
	"Before starting:",
	"  Client 1 unchecks Storm and confirms the lobby status says Storm: Off.",
	"  Select the exact roles above and ready all three clients.",
	"  Watch each repair from the Eluna owner and at least one observer window.",
	"",
	"ER-01 - RMB latch stability:",
	"  Fire Eluna RMB through Ghost at ordinary combat range.",
	"  PASS only if the first enemy contact latches once, the projectile stays at that target instead",
	"  of travelling forward/back repeatedly, the delayed area expands once, and root/cleanup resolve.",
	"",
	"ER-02 - Shift wisp collection and refund:",
	"  Use Ghost to kill Hudson, then keep Eluna outside passive pickup range until positioned.",
	"  Dash Eluna through the allied wisp rather than walking onto it.",
	"  PASS only if the dash path collects the wisp and the used dash is fully refunded immediately.",
	"",
	"ER-03 - Q-started resurrection persistence and enemy cancellation:",
	"  Down Hudson again. Keep Eluna outside passive pickup range. Cast Q, then recast toward the wisp.",
	"  Confirm one Q heal starts resurrection and progress continues smoothly after Q itself expires.",
	"  With no enemy contest, confirm the continuing progress completes the resurrection.",
	"  Repeat after downing Hudson; once Q has started progress, move Ghost onto the wisp.",
	"  PASS only if enemy overlap cancels the healing-started resurrection and decay resumes.",
	"",
	"Do not repeat already accepted Ghost checks, Q-follow movement, or carry-smoothness checks.",
	"Use .\Scripts\Playtest\StopPackagedLocalSmoke.ps1 when finished so all logs are reviewed.",
	"Record PASS/FAIL and concise notes in: $ResultPath"
)
$Instructions | Set-Content -LiteralPath $InstructionsPath -Encoding UTF8

$Record = @(
	"# Eluna Repair Retest",
	"",
	"Session: ``$RunDirectory``",
	"Candidate: verified by launcher preflight; evidence ``$CandidateEvidencePath``",
	"",
	"Use ``PASS`` or ``FAIL``. A clean log review does not replace owner/observer visual confirmation.",
	"",
	"| ID | Check | Result | Notes |",
	"| --- | --- | --- | --- |",
	"| ER-01 | RMB latches first target once; no forward/back loop; root and cleanup resolve | - | |",
	"| ER-02 | Shift path collects allied wisp and fully refunds the used dash | - | |",
	"| ER-03 | Q-started resurrection persists after Q expiry; enemy overlap cancels it | - | |"
)
$Record | Set-Content -LiteralPath $ResultPath -Encoding UTF8

Write-Host ""
Write-Host "Focused Eluna repair retest roles:"
Write-Host "  Client 1: Eluna,  Team 0 / Slot 0"
Write-Host "  Client 2: Hudson, Team 0 / Slot 1 (allied helper)"
Write-Host "  Client 3: Ghost,  Team 1 / Slot 0 (enemy)"
Write-Host "  Client 1: disable Storm before starting"
Write-Host "  Instructions: $InstructionsPath"
Write-Host "  Result record: $ResultPath"
Write-Host "Do not close clients individually; use StopPackagedLocalSmoke.ps1 so logs are reviewed."

exit 0
