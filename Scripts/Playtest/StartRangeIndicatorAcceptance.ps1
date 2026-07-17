param(
	[int]$Port = 7914,
	[int]$ResX = 720,
	[int]$ResY = 405,
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
	-ClientCount 4 `
	-Port $Port `
	-Address "127.0.0.1:$Port" `
	-ResX $ResX `
	-ResY $ResY `
	-OutputRoot $OutputRoot `
	-SessionLabel "Six-hunter and power range-indicator acceptance" `
	-VerifyCandidate:(-not $ValidateOnly) `
	-ValidateOnly:$ValidateOnly

if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}
if ($ValidateOnly) {
	Write-Host "Range-indicator acceptance launcher validation: PASS"
	Write-Host "No session record or process was created."
	$global:LASTEXITCODE = 0
	return
}

$RunDirectory = (Get-Content -LiteralPath (Join-Path $OutputRoot "LatestSession.txt") -Raw).Trim()
$InstructionsPath = Join-Path $RunDirectory "RangeIndicatorInstructions.txt"
$AcceptanceRecordPath = Join-Path $RunDirectory "RangeIndicators-ManualAcceptance.md"

& (Join-Path $PSScriptRoot "NewManualAcceptanceRecord.ps1") `
	-SessionDirectory $RunDirectory `
	-Batch RangeIndicators

$Instructions = @(
	"Breachborne range-indicator manual acceptance",
	"Session: $RunDirectory",
	"",
	"Game 1 roles:",
	"  Client 1: Team 0 / Slot 0 / Ghost (hunter 1)",
	"  Client 2: Team 0 / Slot 1 / Eluna (hunter 3)",
	"  Client 3: Team 1 / Slot 0 / Kingpin (hunter 2)",
	"  Client 4: Team 1 / Slot 1 / Hudson (hunter 4)",
	"",
	"Game 2 roles after normal post-match reset:",
	"  Client 1: Team 0 / Slot 0 / Crysta (hunter 5)",
	"  Client 2: Team 1 / Slot 0 / Void (hunter 6)",
	"  Clients 3 and 4: helpers on opposite teams for death/pawn-replacement checks",
	"",
	"For each hunter, inspect hover and active LMB/RMB/Shift/Q/R previews on the owner.",
	"An observer must confirm the owner's grey range primitives do not replicate.",
	"Test inside and beyond maximum range; compare against docs/ABILITY_RANGE_INDICATORS.md.",
	"Kill or replace at least one previewing pawn and confirm every primitive clears.",
	"Repeat Hudson RMB press/release and Crysta held RMB to verify input release remains intact.",
	"",
	"Power setup from the owning client's console:",
	"  DebugGivePower BungeeShot 1",
	"  DebugGivePower GrapplingHook 2",
	"  DebugGivePower EmergencyPlatform 1",
	"  DebugGivePower RegenerativeArmor 2",
	"  DebugGivePower TacticalNuke 1",
	"Use DebugDropPower 1 or 2 between replacements and DebugPrintPowers to verify F/G slots.",
	"Swap active powers between slots 1 and 2, hover F/G, and confirm metadata follows the slot.",
	"For Tactical Nuke, verify the preview persists until LMB/F confirms and RMB/Escape cancels.",
	"Test a power and hunter ability on cooldown: failed activation must not create a live preview.",
	"",
	"Do not mark RI-01 through RI-04 complete until all six hunters have passed.",
	"Do not mark RI-06 complete until all five built-in powers and both F/G slot positions pass.",
	"A clean log review does not replace owner/observer visual evidence.",
	"",
	"Stop and review:",
	"  .\Scripts\Playtest\StopPackagedLocalSmoke.ps1",
	"  Record results: $AcceptanceRecordPath"
)
$Instructions | Set-Content -LiteralPath $InstructionsPath -Encoding UTF8

Write-Host ""
Write-Host "Range-indicator acceptance session ready."
Write-Host "  Game 1: Ghost/Eluna versus Kingpin/Hudson"
Write-Host "  Game 2: Crysta versus Void plus lifecycle helpers"
Write-Host "  Instructions: $InstructionsPath"
Write-Host "  Result record: $AcceptanceRecordPath"
Write-Host "Do not close clients individually; use StopPackagedLocalSmoke.ps1 so logs are reviewed."

exit 0
