param(
	[int]$Port = 7871,
	[int]$ConnectionSeconds = 65,
	[string]$ClientBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsClient",
	[string]$ServerBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsServer",
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedWispRules"
)

$ErrorActionPreference = "Stop"
$HunterA = 3
$HunterB = 1
$Scenarios = @(
	"natural",
	"ally",
	"ally_enemy",
	"enemy",
	"healing",
	"healing_latched",
	"healing_enemy",
	"carried_enemy",
	"eluna_shift_pickup",
	"eluna_pickup",
	"eluna_cc_drop",
	"eluna_r_revive"
)

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
	-ServerExtraArgs @("-BBDeathSmoke", "-BBWispRulesSmoke") `
	-ClientExtraArgs @("-BBDeathSmoke", "-BBWispRulesSmoke")
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$RunDirectory = Get-ChildItem -LiteralPath $OutputRoot -Directory |
	Where-Object { -not $ExistingRunDirectories.ContainsKey($_.FullName) } |
	Sort-Object CreationTime -Descending |
	Select-Object -First 1
if (-not $RunDirectory) {
	throw "A new wisp-rules evidence directory was not created."
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
	$Failures += "The initial Eluna/Ghost ability lifecycle did not complete."
}
if ($ServerText -match "BB_DEATH_SMOKE\|SERVER_FAIL" -or
	$ServerText -match "BB_WISP_RULES\|SERVER_FAIL") {
	$Failures += "The server reported a death or wisp-rules smoke failure."
}
if ($ServerText -notmatch "BB_DEATH_SMOKE\|SERVER_RESULT\|.*wisp=1 cleanup=1 states=1") {
	$Failures += "The authoritative death did not produce a cleanly possessed wisp."
}
if ($Client2Text -notmatch "BB_DEATH_SMOKE\|CLIENT_WISP\|index=2") {
	$Failures += "The victim client did not observe wisp possession."
}

foreach ($Scenario in $Scenarios) {
	if ($ServerText -notmatch "BB_WISP_RULES\|SERVER_RESULT\|scenario=$Scenario .*success=1") {
		$Failures += "Wisp scenario '$Scenario' did not pass."
	}
}
if ($ServerText -notmatch "BB_WISP_RULES\|SERVER_COMPLETE\|passed=12 total=12") {
	$Failures += "The server did not complete all 12 wisp-rule scenarios."
}
if ($ServerText -notmatch "Eluna R: CHANNEL STARTED \| Target=BBWispPawn" -or
	$ServerText -notmatch "WispRevive: player=.* health=.* passives=1") {
	$Failures += "Eluna R did not complete the authoritative revive with a restarted passive."
}
if ($Client2Text -notmatch "BB_DEATH_SMOKE\|CLIENT_REVIVED\|index=2 pawn=.* alive=1") {
	$Failures += "The victim client did not observe hunter repossession after the rule matrix."
}

$Result = if ($Failures.Count -eq 0) { "PASS" } else { "FAIL" }
$Summary = @(
	"Breachborne packaged wisp-rules smoke: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Hunters: Eluna (3) downs Ghost (1)",
	"Dedicated clients joined: $(if ($Client1Text -match 'BB_ABILITY_SMOKE\|COMPLETE' -and $Client2Text -match 'BB_ABILITY_SMOKE\|COMPLETE') { 'PASS' } else { 'FAIL' })",
	"Death/wisp possession: $(if ($ServerText -match 'BB_DEATH_SMOKE\|SERVER_RESULT\|.*wisp=1 cleanup=1 states=1') { 'PASS' } else { 'FAIL' })"
)
foreach ($Scenario in $Scenarios) {
	$ScenarioResult = if ($ServerText -match "BB_WISP_RULES\|SERVER_RESULT\|scenario=$Scenario .*success=1") { "PASS" } else { "FAIL" }
	$Summary += "Wisp rule $Scenario`: $ScenarioResult"
}
$Summary += @(
	"Eluna R revive/re-possession: $(if ($Client2Text -match 'BB_DEATH_SMOKE\|CLIENT_REVIVED') { 'PASS' } else { 'FAIL' })",
	"Evidence: $($RunDirectory.FullName)"
)
if ($Failures.Count -gt 0) {
	$Summary += "Failures:"
	$Summary += $Failures | ForEach-Object { "- $_" }
}
$Summary | Set-Content -LiteralPath (Join-Path $RunDirectory.FullName "WispRulesSummary.txt") -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Failures.Count -gt 0) {
	exit 1
}
