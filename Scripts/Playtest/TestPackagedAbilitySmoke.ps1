param(
	[int]$HunterA = 1,
	[int]$HunterB = 2,
	[int]$Port = 7801,
	[int]$ConnectionSeconds = 38,
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedAbilitySmoke",
	[string[]]$ServerExtraArgs = @(),
	[string[]]$ClientExtraArgs = @()
)

$ErrorActionPreference = "Stop"

foreach ($HunterID in @($HunterA, $HunterB)) {
	if ($HunterID -lt 1 -or $HunterID -gt 6) {
		throw "Hunter IDs must be between 1 and 6."
	}
}

$HandshakeScript = Join-Path $PSScriptRoot "TestPackagedHeadlessHandshake.ps1"
& $HandshakeScript `
	-Port $Port `
	-ClientCount 2 `
	-ServerStartupSeconds 4 `
	-ConnectionSeconds $ConnectionSeconds `
	-OutputRoot $OutputRoot `
	-AbilitySmoke `
	-SmokeHunterIDs @($HunterA, $HunterB) `
	-ServerExtraArgs $ServerExtraArgs `
	-ClientExtraArgs $ClientExtraArgs
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$RunDirectory = Get-ChildItem -LiteralPath $OutputRoot -Directory |
	Sort-Object LastWriteTime -Descending |
	Select-Object -First 1
if (-not $RunDirectory) {
	throw "Ability smoke evidence directory was not created."
}

$Failures = @()
$RequiredActions = @("LMB_PRESS", "RMB_PRESS", "SHIFT", "Q_PRESS", "R")
for ($Index = 1; $Index -le 2; $Index++) {
	$HunterID = @($HunterA, $HunterB)[$Index - 1]
	$ClientLog = Join-Path $RunDirectory.FullName "Client$Index.log"
	$ClientText = Get-Content -LiteralPath $ClientLog -Raw
	if ($ClientText -notmatch "BB_ABILITY_SMOKE\|COMPLETE\|index=$Index hunter=$HunterID") {
		$Failures += "Client $Index / hunter $HunterID did not complete the ability sequence."
	}
	foreach ($Action in $RequiredActions) {
		$Pattern = "BB_ABILITY_SMOKE\|ACTIVATE\|index=$Index hunter=$HunterID action=$Action .*success=1"
		if ($ClientText -notmatch $Pattern) {
			$Failures += "Client $Index / hunter $HunterID did not successfully activate $Action."
		}
	}
	if ($ClientText -notmatch "BB_ABILITY_SMOKE\|RELEASE\|index=$Index hunter=$HunterID input=LMB") {
		$Failures += "Client $Index / hunter $HunterID did not release LMB."
	}
	if ($ClientText -notmatch "BB_ABILITY_SMOKE\|RELEASE\|index=$Index hunter=$HunterID input=RMB") {
		$Failures += "Client $Index / hunter $HunterID did not release RMB."
	}
}

$ServerText = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "Server.log") -Raw
$PreparedCount = ([regex]::Matches($ServerText, "BB_ABILITY_SMOKE\|SERVER_PREPARED")).Count
if ($PreparedCount -ne 2) {
	$Failures += "Expected two server-prepared smoke players; observed $PreparedCount."
}
if ($ServerText -notmatch "BB_NETFLOW\|SERVER\|StartGameplayMatch players=2") {
	$Failures += "Server did not start a two-player gameplay match."
}

$Result = if ($Failures.Count -eq 0) { "PASS" } else { "FAIL" }
$Summary = @(
	"Breachborne packaged ability smoke: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Hunters: $HunterA, $HunterB",
	"Port: $Port",
	"Server extra args: $($ServerExtraArgs -join ' ')",
	"Client extra args: $($ClientExtraArgs -join ' ')",
	"Server-prepared players: $PreparedCount",
	"Evidence: $($RunDirectory.FullName)"
)
if ($Failures.Count -gt 0) {
	$Summary += "Failures:"
	$Summary += $Failures | ForEach-Object { "- $_" }
}
$Summary | Set-Content -LiteralPath (Join-Path $RunDirectory.FullName "AbilitySummary.txt") -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Failures.Count -gt 0) {
	exit 1
}
