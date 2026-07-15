param(
	[int]$Port = 7971,
	[int]$ConnectionSeconds = 38,
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedMatchReset"
)

$ErrorActionPreference = "Stop"
$ExistingRuns = @{}
if (Test-Path -LiteralPath $OutputRoot) {
	Get-ChildItem -LiteralPath $OutputRoot -Directory | ForEach-Object {
		$ExistingRuns[$_.FullName] = $true
	}
}

& (Join-Path $PSScriptRoot "TestPackagedAbilitySmoke.ps1") `
	-HunterA 3 `
	-HunterB 1 `
	-Port $Port `
	-ConnectionSeconds $ConnectionSeconds `
	-OutputRoot $OutputRoot `
	-ServerExtraArgs @("-BBMatchResetSmoke")
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$RunDirectory = Get-ChildItem -LiteralPath $OutputRoot -Directory |
	Where-Object { -not $ExistingRuns.ContainsKey($_.FullName) } |
	Sort-Object CreationTime -Descending |
	Select-Object -First 1
if (-not $RunDirectory) {
	throw "A new match-reset smoke evidence directory was not created."
}

$ServerText = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "Server.log") -Raw
$GhostClientText = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "Client2.log") -Raw
$AbilitySummaryText = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "AbilitySummary.txt") -Raw
$Failures = @()

$InjectedCount = ([regex]::Matches($ServerText, "BB_MATCH_RESET_SMOKE\|SERVER\|Injected")).Count
$CleanResetCount = ([regex]::Matches(
	$ServerText,
	"BB_MATCH_RESET\|SERVER\|AbilitySystem .*dead_before=1 wisp_before=1 dead_after=0 wisp_after=0")).Count
if ($InjectedCount -ne 2) {
	$Failures += "Expected stale state injection for two players; observed $InjectedCount."
}
if ($CleanResetCount -ne 2) {
	$Failures += "Expected two clean server reset reports; observed $CleanResetCount."
}
if ($ServerText -notmatch "BB_MATCH_RESET_SMOKE\|SERVER\|Injected .*hunter=1 .*ghost_shift_cd=1") {
	$Failures += "Ghost did not receive the stale Shift cooldown fixture."
}
if ($AbilitySummaryText -notmatch "packaged ability smoke:\s+PASS") {
	$Failures += "The underlying Eluna/Ghost ability lifecycle did not pass."
}
foreach ($Action in @("LMB_PRESS", "RMB_PRESS", "SHIFT", "Q_PRESS", "R")) {
	if ($GhostClientText -notmatch "BB_ABILITY_SMOKE\|ACTIVATE\|index=2 hunter=1 action=$Action .*success=1") {
		$Failures += "Ghost did not activate $Action after stale-state cleanup."
	}
}

$Result = if ($Failures.Count -eq 0) { "PASS" } else { "FAIL" }
$Summary = @(
	"Breachborne packaged match-reset smoke: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Fixture: stale Dead + Wisp on both players; stale Ghost Shift cooldown",
	"Clean server resets: $CleanResetCount/2",
	"Ghost post-reset activations: $(if ($Failures.Count -eq 0) { '5/5' } else { 'FAIL' })",
	"Evidence: $($RunDirectory.FullName)"
)
if ($Failures.Count -gt 0) {
	$Summary += "Failures:"
	$Summary += $Failures | ForEach-Object { "- $_" }
}
$Summary | Set-Content -LiteralPath (Join-Path $RunDirectory.FullName "MatchResetSummary.txt") -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Failures.Count -gt 0) {
	exit 1
}
