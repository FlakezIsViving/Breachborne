param(
	[int]$Port = 7911,
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

$StartScript = Join-Path $PSScriptRoot "StartPackagedLocalSmoke.ps1"
$global:LASTEXITCODE = 0
& $StartScript `
	-ClientCount 3 `
	-Port $Port `
	-Address "127.0.0.1:$Port" `
	-ResX $ResX `
	-ResY $ResY `
	-OutputRoot $OutputRoot `
	-SessionLabel "Ghost/Eluna owner-observer acceptance" `
	-VerifyCandidate:(-not $ValidateOnly) `
	-ValidateOnly:$ValidateOnly

if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}
if ($ValidateOnly) {
	Write-Host "Ghost/Eluna acceptance launcher validation: PASS"
	Write-Host "No session record or process was created."
	$global:LASTEXITCODE = 0
	return
}

$LatestSessionPath = Join-Path $OutputRoot "LatestSession.txt"
$RunDirectory = (Get-Content -LiteralPath $LatestSessionPath -Raw).Trim()
$InstructionsPath = Join-Path $RunDirectory "GhostElunaInstructions.txt"
$AcceptanceRecordPath = Join-Path $RunDirectory "GhostEluna-ManualAcceptance.md"
$WispAcceptanceRecordPath = Join-Path $RunDirectory "WispUI-ManualAcceptance.md"

& (Join-Path $PSScriptRoot "NewManualAcceptanceRecord.ps1") `
	-SessionDirectory $RunDirectory `
	-Batch GhostEluna
& (Join-Path $PSScriptRoot "NewManualAcceptanceRecord.ps1") `
	-SessionDirectory $RunDirectory `
	-Batch WispUI

$Instructions = @(
	"Breachborne Ghost/Eluna manual acceptance",
	"Session: $RunDirectory",
	"",
	"Lobby roles:",
	"  Client 1: bottom-left / displayed Team 1 Slot 1 / Eluna (internal team 0, hunter 3)",
	"  Client 2: top-left / displayed Team 1 Slot 2 / Hudson (internal team 0, hunter 4, allied helper)",
	"  Client 3: top-right / displayed Team 2 Slot 1 / Ghost (internal team 1, hunter 1)",
	"",
	"On Client 1, uncheck the Storm lobby checkbox before starting the match.",
	"Confirm the lobby status reads Storm: Off, then select the roles above and ready all three clients.",
	"Keep Client 2 near Eluna so the five-second passive heal pulse can be checked.",
	"Use Ghost to damage the helper below full health for Eluna heal/R checks.",
	"Use Ghost to kill the helper for Ghost's reset and Eluna's wisp pickup/R checks.",
	"Watch each ability from its owner and from at least one other client.",
	"",
	"Priority regression checks:",
	"  1. HUD displays Eluna, Hudson, and Ghost rather than Hunter 3/4.",
	"  2. Eluna passive real-heal feedback is a restrained floating green plus, not a floor disc.",
	"  3. Ghost Q has a readable ~0.3 second warning before trace/damage.",
	"  4. Ghost R has a readable ~0.75 second warning before landing/damage.",
	"  5. Two Ghost kills each produce exactly one passive pulse and refresh Ghost's active cooldowns.",
	"  6. Initial Eluna Q follows Eluna while she moves.",
	"  7. Recast Q travels only to its range limit, selects the first eligible allied hunter/wisp, and follows it.",
	"  8. Q attached to a killed hunter drops at the death location; Q healing starts wisp resurrection.",
	"  9. Dashing Eluna through the allied wisp collects it and fully refunds that dash.",
	" 10. Enemy contest overrides Q/healing resurrection progress.",
	" 11. Full Eluna R channel revives; walking out of range or enemy CC cancels and cleans the tether.",
	"",
	"Full GE checks: Ghost and Eluna LMB/RMB/Shift/Q/R/passive from owner and observer views.",
	"Full WI checks: both bars; natural, ally, healing, enemy priority, and Eluna carry/CC-drop states.",
	"Compare focused owner movement with observer windows and record jitter separately from ability VFX.",
	"",
	"Stop and review:",
	"  .\Scripts\Playtest\StopPackagedLocalSmoke.ps1",
	"  Ghost/Eluna results: $AcceptanceRecordPath",
	"  Wisp UI results: $WispAcceptanceRecordPath",
	"",
	"Do not mark a row PASS unless owner/observer presentation, gameplay result, and cleanup agree."
)
$Instructions | Set-Content -LiteralPath $InstructionsPath -Encoding UTF8

Write-Host ""
Write-Host "Ghost/Eluna acceptance roles:"
Write-Host "  Client 1: Eluna,  Team 0 / Slot 0"
Write-Host "  Client 2: Hudson, Team 0 / Slot 1 (helper)"
Write-Host "  Client 3: Ghost,  Team 1 / Slot 0"
Write-Host "  Client 1: uncheck Storm and confirm the lobby status says Storm: Off before start"
Write-Host "  Instructions: $InstructionsPath"
Write-Host "  Ghost/Eluna record: $AcceptanceRecordPath"
Write-Host "  Wisp UI record: $WispAcceptanceRecordPath"
Write-Host "Do not close clients individually; use StopPackagedLocalSmoke.ps1 so logs are reviewed."

exit 0
