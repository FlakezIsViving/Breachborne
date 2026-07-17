param(
	[int]$Port = 7912,
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
	-SessionLabel "Kingpin/Hudson owner-observer acceptance" `
	-VerifyCandidate:(-not $ValidateOnly) `
	-ValidateOnly:$ValidateOnly

if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}
if ($ValidateOnly) {
	Write-Host "Kingpin/Hudson acceptance launcher validation: PASS"
	Write-Host "No session record or process was created."
	$global:LASTEXITCODE = 0
	return
}

$RunDirectory = (Get-Content -LiteralPath (Join-Path $OutputRoot "LatestSession.txt") -Raw).Trim()
$InstructionsPath = Join-Path $RunDirectory "KingpinHudsonInstructions.txt"
$AcceptanceRecordPath = Join-Path $RunDirectory "KingpinHudson-ManualAcceptance.md"

& (Join-Path $PSScriptRoot "NewManualAcceptanceRecord.ps1") `
	-SessionDirectory $RunDirectory `
	-Batch KingpinHudson

$Instructions = @(
	"Breachborne Kingpin/Hudson manual acceptance",
	"Session: $RunDirectory",
	"",
	"Lobby roles:",
	"  Client 1: Team 0 / Slot 0 / Kingpin (hunter 2)",
	"  Client 2: Team 1 / Slot 0 / Hudson (hunter 4)",
	"  Client 3: Team 1 / Slot 1 / Ghost (hunter 1, second enemy target)",
	"",
	"Keep Clients 2 and 3 close together for Kingpin LMB chain and multi-target sector checks.",
	"Watch each ability from its owner and from at least one other client.",
	"Hold Hudson LMB for ten seconds normally and again after a one-second RMB spin-up.",
	"Release Hudson RMB and Shift explicitly; verify their loop visuals and state end.",
	"",
	"Kingpin checks: LMB chain, RMB, Shift, Q, two-shell R, passive CC response.",
	"Hudson checks: RMB, sustained LMB, Shift, Q, R, passive repair/heal.",
	"",
	"Stop and review:",
	"  .\Scripts\Playtest\StopPackagedLocalSmoke.ps1",
	"  Record results: $AcceptanceRecordPath"
)
$Instructions | Set-Content -LiteralPath $InstructionsPath -Encoding UTF8

Write-Host ""
Write-Host "Kingpin/Hudson acceptance roles:"
Write-Host "  Client 1: Kingpin, Team 0 / Slot 0"
Write-Host "  Client 2: Hudson,  Team 1 / Slot 0"
Write-Host "  Client 3: Ghost,   Team 1 / Slot 1 (second enemy target)"
Write-Host "  Instructions: $InstructionsPath"
Write-Host "  Result record: $AcceptanceRecordPath"
Write-Host "Do not close clients individually; use StopPackagedLocalSmoke.ps1 so logs are reviewed."

exit 0
