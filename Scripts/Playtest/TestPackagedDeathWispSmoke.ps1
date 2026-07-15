param(
	[int]$HunterA = 1,
	[int]$HunterB = 2,
	[int]$Port = 7841,
	[int]$ConnectionSeconds = 45,
	[string]$ClientBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsClient",
	[string]$ServerBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsServer",
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedDeathWisp"
)

$ErrorActionPreference = "Stop"

foreach ($HunterID in @($HunterA, $HunterB)) {
	if ($HunterID -lt 1 -or $HunterID -gt 6) {
		throw "Hunter IDs must be between 1 and 6."
	}
}

$ExistingRunDirectories = @{}
if (Test-Path -LiteralPath $OutputRoot) {
	Get-ChildItem -LiteralPath $OutputRoot -Directory | ForEach-Object {
		$ExistingRunDirectories[$_.FullName] = $true
	}
}

& (Join-Path $PSScriptRoot "TestPackagedHeadlessHandshake.ps1") `
	-Port $Port `
	-ClientCount 2 `
	-ServerStartupSeconds 4 `
	-ConnectionSeconds $ConnectionSeconds `
	-OutputRoot $OutputRoot `
	-ClientBuildRoot $ClientBuildRoot `
	-ServerBuildRoot $ServerBuildRoot `
	-AbilitySmoke `
	-SmokeHunterIDs @($HunterA, $HunterB) `
	-ServerExtraArgs @("-BBDeathSmoke") `
	-ClientExtraArgs @("-BBDeathSmoke")
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$RunDirectory = Get-ChildItem -LiteralPath $OutputRoot -Directory |
	Where-Object { -not $ExistingRunDirectories.ContainsKey($_.FullName) } |
	Sort-Object CreationTime -Descending |
	Select-Object -First 1
if (-not $RunDirectory) {
	throw "A new death/wisp smoke evidence directory was not created."
}

& (Join-Path $PSScriptRoot "ReviewInteractivePlaytest.ps1") -SessionDirectory $RunDirectory.FullName
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$ServerText = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "Server.log") -Raw
$Client1Text = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "Client1.log") -Raw
$Client2Text = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "Client2.log") -Raw
$Failures = @()

if ($Client1Text -notmatch "BB_ABILITY_SMOKE\|COMPLETE\|index=1 hunter=$HunterA" -or
	$Client2Text -notmatch "BB_ABILITY_SMOKE\|COMPLETE\|index=2 hunter=$HunterB") {
	$Failures += "The initial hunter $HunterA/$HunterB ability lifecycle did not complete."
}
if ($ServerText -match "BB_DEATH_SMOKE\|SERVER_FAIL") {
	$Failures += "The server reported a death-smoke failure."
}
if ($ServerText -notmatch "HealthSet: >>> HEALTH DEPLETED") {
	$Failures += "Lethal damage did not pass through HealthSet depletion."
}
if ($ServerText -notmatch "WispSpawn: victim=.* wisp=.* controller=") {
	$Failures += "The server did not log a spawned and possessed wisp."
}
if ($ServerText -notmatch "BB_DEATH_SMOKE\|SERVER_RESULT\|.*health_after=0\.0 alive=0 .*wisp=1 cleanup=1 states=1") {
	$Failures += "The authoritative death result did not report death, wisp possession, owner-actor cleanup, and transient-state cleanup."
}
if ($Client2Text -notmatch "BB_DEATH_SMOKE\|CLIENT_WISP\|index=2 pawn=.* owner=.* hp=.* rez=") {
	$Failures += "The victim client did not observe possession of its replicated wisp."
}
if ($ServerText -notmatch "Wisp: REVIVE COMPLETE") {
	$Failures += "Repeated healing did not complete the wisp revive bar."
}
if ($ServerText -notmatch "WispRevive: player=.* health=") {
	$Failures += "The server did not restore the downed hunter."
}
if ($ServerText -notmatch "WispRevive: player=.* health=.* passives=1") {
	$Failures += "The revived hunter did not restart exactly one server passive."
}
if ($Client2Text -notmatch "BB_DEATH_SMOKE\|CLIENT_REVIVED\|index=2 pawn=.* health=.* alive=1") {
	$Failures += "The victim client did not observe restored hunter possession."
}

$Result = if ($Failures.Count -eq 0) { "PASS" } else { "FAIL" }
$Summary = @(
	"Breachborne packaged death/wisp smoke: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Hunters: $HunterA kills $HunterB",
	"Lethal GAS damage: $(if ($ServerText -match 'HealthSet: >>> HEALTH DEPLETED') { 'PASS' } else { 'FAIL' })",
	"Server wisp spawn/possession: $(if ($ServerText -match 'BB_DEATH_SMOKE\|SERVER_RESULT\|.*wisp=1') { 'PASS' } else { 'FAIL' })",
	"Owned actor/transient state cleanup: $(if ($ServerText -match 'BB_DEATH_SMOKE\|SERVER_RESULT\|.*cleanup=1 states=1') { 'PASS' } else { 'FAIL' })",
	"Victim client wisp observation: $(if ($Client2Text -match 'BB_DEATH_SMOKE\|CLIENT_WISP') { 'PASS' } else { 'FAIL' })",
	"Healing-driven revive: $(if ($ServerText -match 'Wisp: REVIVE COMPLETE') { 'PASS' } else { 'FAIL' })",
	"Server passive restart: $(if ($ServerText -match 'WispRevive: player=.*passives=1') { 'PASS' } else { 'FAIL' })",
	"Victim client hunter repossession: $(if ($Client2Text -match 'BB_DEATH_SMOKE\|CLIENT_REVIVED') { 'PASS' } else { 'FAIL' })",
	"Evidence: $($RunDirectory.FullName)"
)
if ($Failures.Count -gt 0) {
	$Summary += "Failures:"
	$Summary += $Failures | ForEach-Object { "- $_" }
}
$Summary | Set-Content -LiteralPath (Join-Path $RunDirectory.FullName "DeathWispSummary.txt") -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Failures.Count -gt 0) {
	exit 1
}
