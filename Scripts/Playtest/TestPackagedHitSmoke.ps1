param(
	[int]$HunterA = 1,
	[int]$HunterB = 2,
	[int]$Port = 7851,
	[int]$ConnectionSeconds = 50,
	[string]$ClientBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsClient",
	[string]$ServerBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsServer",
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedHitSmoke"
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
	-ClientBuildRoot $ClientBuildRoot `
	-ServerBuildRoot $ServerBuildRoot `
	-OutputRoot $OutputRoot `
	-AbilitySmoke `
	-SmokeHunterIDs @($HunterA, $HunterB) `
	-ServerExtraArgs @("-BBHitSmoke") `
	-ClientExtraArgs @("-BBHitSmoke")
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$RunDirectory = Get-ChildItem -LiteralPath $OutputRoot -Directory |
	Where-Object { -not $ExistingRunDirectories.ContainsKey($_.FullName) } |
	Sort-Object CreationTime -Descending |
	Select-Object -First 1
if (-not $RunDirectory) {
	throw "A new hit-smoke evidence directory was not created."
}

& (Join-Path $PSScriptRoot "ReviewInteractivePlaytest.ps1") -SessionDirectory $RunDirectory.FullName
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$ServerText = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "Server.log") -Raw
$Failures = @()
if ($ServerText -notmatch "BB_HIT_SMOKE\|SERVER_PREPARED\|players=2") {
	$Failures += "The server did not prepare both hit-smoke players."
}
if ($ServerText -match "BB_HIT_SMOKE\|SERVER_REPORT_FAIL") {
	$Failures += "The server reported a hit-smoke setup failure."
}

for ($Index = 1; $Index -le 2; $Index++) {
	$HunterID = @($HunterA, $HunterB)[$Index - 1]
	$ClientText = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "Client$Index.log") -Raw
	if ($ClientText -notmatch "BB_ABILITY_SMOKE\|COMPLETE\|index=$Index hunter=$HunterID") {
		$Failures += "Client $Index / hunter $HunterID did not complete the base ability lifecycle."
	}
	if ($ClientText -notmatch "BB_ABILITY_SMOKE\|ACTIVATE\|index=$Index hunter=$HunterID action=HIT_LMB .*success=1") {
		$Failures += "Client $Index / hunter $HunterID did not activate the hit-test LMB."
	}
	if ($ClientText -notmatch "BB_HIT_SMOKE\|CLIENT_COMPLETE\|index=$Index hunter=$HunterID") {
		$Failures += "Client $Index / hunter $HunterID did not complete the hit-test sequence."
	}
	if ($ServerText -notmatch "BB_HIT_SMOKE\|SERVER_REPORT\|attacker=$Index hunter=$HunterID .*success=1") {
		$Failures += "Hunter $HunterID did not produce authoritative LMB health loss."
	}
}

$Result = if ($Failures.Count -eq 0) { "PASS" } else { "FAIL" }
$Summary = @(
	"Breachborne packaged LMB hit smoke: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Hunters: $HunterA, $HunterB",
	"Authoritative LMB damage reports: $(([regex]::Matches($ServerText, 'BB_HIT_SMOKE\|SERVER_REPORT\|.*success=1')).Count)/2",
	"Evidence: $($RunDirectory.FullName)"
)
if ($Failures.Count -gt 0) {
	$Summary += "Failures:"
	$Summary += $Failures | ForEach-Object { "- $_" }
}
$Summary | Set-Content -LiteralPath (Join-Path $RunDirectory.FullName "HitSummary.txt") -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Failures.Count -gt 0) {
	exit 1
}
