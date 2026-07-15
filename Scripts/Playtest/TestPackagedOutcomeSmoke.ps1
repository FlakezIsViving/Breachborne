param(
	[int]$HunterID = 1,
	[int]$TargetHunterID = 2,
	[int]$Port = 7861,
	[int]$ConnectionSeconds = 55,
	[string]$ClientBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsClient",
	[string]$ServerBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsServer",
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedOutcomeSmoke",
	[string[]]$ServerExtraArgs = @(),
	[string[]]$ClientExtraArgs = @()
)

$ErrorActionPreference = "Stop"

foreach ($CandidateID in @($HunterID, $TargetHunterID)) {
	if ($CandidateID -lt 1 -or $CandidateID -gt 6) {
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
	-ClientBuildRoot $ClientBuildRoot `
	-ServerBuildRoot $ServerBuildRoot `
	-OutputRoot $OutputRoot `
	-AbilitySmoke `
	-SmokeHunterIDs @($HunterID, $TargetHunterID) `
	-ServerExtraArgs (@("-BBOutcomeSmoke") + $ServerExtraArgs) `
	-ClientExtraArgs (@("-BBOutcomeSmoke") + $ClientExtraArgs)
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$RunDirectory = Get-ChildItem -LiteralPath $OutputRoot -Directory |
	Where-Object { -not $ExistingRunDirectories.ContainsKey($_.FullName) } |
	Sort-Object CreationTime -Descending |
	Select-Object -First 1
if (-not $RunDirectory) {
	throw "A new outcome-smoke evidence directory was not created."
}

& (Join-Path $PSScriptRoot "ReviewInteractivePlaytest.ps1") -SessionDirectory $RunDirectory.FullName
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$ServerText = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "Server.log") -Raw
$AttackerText = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "Client1.log") -Raw
$TargetText = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "Client2.log") -Raw
$Failures = @()
$Stages = @("RMB", "SHIFT", "Q", "R")

if ($AttackerText -notmatch "BB_OUTCOME_SMOKE\|CLIENT_COMPLETE\|index=1 hunter=$HunterID stages=4") {
	$Failures += "Attacker hunter $HunterID did not complete all four outcome stages."
}
if ($TargetText -notmatch "BB_ABILITY_SMOKE\|READY\|index=2 hunter=$TargetHunterID") {
	$Failures += "Target hunter $TargetHunterID was not ready."
}
if ($ServerText -match "BB_OUTCOME_SMOKE\|SERVER_PREPARE_FAIL") {
	$Failures += "The server reported an outcome-smoke preparation failure."
}

foreach ($Stage in $Stages) {
	if ($AttackerText -notmatch "BB_ABILITY_SMOKE\|ACTIVATE\|index=1 hunter=$HunterID action=OUTCOME_$Stage .*success=1") {
		$Failures += "Hunter $HunterID did not activate $Stage for the outcome pass."
	}
	if ($ServerText -notmatch "BB_OUTCOME_SMOKE\|SERVER_PREPARED\|hunter=$HunterID stage=$Stage ") {
		$Failures += "The server did not prepare hunter $HunterID $Stage."
	}
	if ($ServerText -notmatch "BB_OUTCOME_SMOKE\|SERVER_REPORT\|hunter=$HunterID stage=$Stage .*success=1") {
		$Failures += "Hunter $HunterID $Stage did not meet its authoritative outcome contract."
	}
}

$PassCount = ([regex]::Matches(
	$ServerText,
	"BB_OUTCOME_SMOKE\|SERVER_REPORT\|hunter=$HunterID stage=(RMB|SHIFT|Q|R) .*success=1"
)).Count
$Result = if ($Failures.Count -eq 0) { "PASS" } else { "FAIL" }
$Summary = @(
	"Breachborne packaged non-LMB outcome smoke: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Attacker hunter: $HunterID",
	"Target hunter: $TargetHunterID",
	"Server extra args: $($ServerExtraArgs -join ' ')",
	"Client extra args: $($ClientExtraArgs -join ' ')",
	"Authoritative outcome contracts: $PassCount/4",
	"Evidence: $($RunDirectory.FullName)"
)
if ($Failures.Count -gt 0) {
	$Summary += "Failures:"
	$Summary += $Failures | ForEach-Object { "- $_" }
}
$Summary | Set-Content -LiteralPath (Join-Path $RunDirectory.FullName "OutcomeSummary.txt") -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Failures.Count -gt 0) {
	exit 1
}

exit 0
