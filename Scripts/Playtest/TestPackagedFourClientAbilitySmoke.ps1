param(
	[int]$Port = 7901,
	[int]$ConnectionSeconds = 50,
	[string]$ClientBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsClient",
	[string]$ServerBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsServer",
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedFourClientAbilitySmoke"
)

$ErrorActionPreference = "Stop"
$HunterIDs = @(1, 2, 3, 4)
$RequiredActions = @("LMB_PRESS", "RMB_PRESS", "SHIFT", "Q_PRESS", "R")

$ExistingRunDirectories = @{}
if (Test-Path -LiteralPath $OutputRoot) {
	Get-ChildItem -LiteralPath $OutputRoot -Directory | ForEach-Object {
		$ExistingRunDirectories[$_.FullName] = $true
	}
}

& (Join-Path $PSScriptRoot "TestPackagedHeadlessHandshake.ps1") `
	-Port $Port `
	-ClientCount 4 `
	-ServerStartupSeconds 4 `
	-ConnectionSeconds $ConnectionSeconds `
	-OutputRoot $OutputRoot `
	-ClientBuildRoot $ClientBuildRoot `
	-ServerBuildRoot $ServerBuildRoot `
	-AbilitySmoke `
	-SmokeHunterIDs $HunterIDs `
	-WaitForClientJoinBeforeNext `
	-ServerExtraArgs @("-BBFourClientSmoke") `
	-ClientExtraArgs @("-BBFourClientSmoke")
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$RunDirectory = Get-ChildItem -LiteralPath $OutputRoot -Directory |
	Where-Object { -not $ExistingRunDirectories.ContainsKey($_.FullName) } |
	Sort-Object CreationTime -Descending |
	Select-Object -First 1
if (-not $RunDirectory) {
	throw "A new four-client ability-smoke evidence directory was not created."
}

& (Join-Path $PSScriptRoot "ReviewInteractivePlaytest.ps1") -SessionDirectory $RunDirectory.FullName
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$Failures = @()
$SuccessfulActivations = 0
$SuccessfulReleases = 0
for ($Index = 1; $Index -le 4; $Index++) {
	$HunterID = $HunterIDs[$Index - 1]
	$ClientText = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "Client$Index.log") -Raw
	if ($ClientText -notmatch "BB_ABILITY_SMOKE\|CONFIG\|index=$Index hunter=$HunterID four_client=1") {
		$Failures += "Client $Index did not enable the four-client smoke path."
	}
	if ($ClientText -notmatch "BB_ABILITY_SMOKE\|COMPLETE\|index=$Index hunter=$HunterID") {
		$Failures += "Client $Index / hunter $HunterID did not complete the ability lifecycle."
	}
	foreach ($Action in $RequiredActions) {
		if ($ClientText -notmatch "BB_ABILITY_SMOKE\|ACTIVATE\|index=$Index hunter=$HunterID action=$Action .*success=1") {
			$Failures += "Client $Index / hunter $HunterID did not successfully activate $Action."
		}
		else {
			$SuccessfulActivations++
		}
	}
	foreach ($InputName in @("LMB", "RMB")) {
		if ($ClientText -notmatch "BB_ABILITY_SMOKE\|RELEASE\|index=$Index hunter=$HunterID input=$InputName") {
			$Failures += "Client $Index / hunter $HunterID did not release $InputName."
		}
		else {
			$SuccessfulReleases++
		}
	}
}

$ServerText = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "Server.log") -Raw
$PreparedCount = ([regex]::Matches($ServerText, "BB_ABILITY_SMOKE\|SERVER_PREPARED")).Count
if ($PreparedCount -ne 4) {
	$Failures += "Expected four server-prepared players; observed $PreparedCount."
}
if ($ServerText -notmatch "BB_NETFLOW\|SERVER\|StartGameplayMatch players=4") {
	$Failures += "The server did not start a four-player gameplay match."
}

$ValidMappings = 0
for ($Index = 1; $Index -le 4; $Index++) {
	$HunterID = $HunterIDs[$Index - 1]
	$TeamID = [math]::Floor(($Index - 1) / 2)
	$SlotIndex = ($Index - 1) % 2
	$Pattern = "BB_ABILITY_SMOKE\|SERVER_PREPARED\|index=$Index hunter=$HunterID team=$TeamID slot=$SlotIndex"
	if ($ServerText -notmatch $Pattern) {
		$Failures += "Client $Index did not reach gameplay as team $TeamID / slot $SlotIndex / hunter $HunterID."
	}
	else {
		$ValidMappings++
	}
}

$Result = if ($Failures.Count -eq 0) { "PASS" } else { "FAIL" }
$Summary = @(
	"Breachborne packaged four-client 2v2 ability smoke: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Hunters: 1, 2 versus 3, 4",
	"Team 0 slots: clients 1-2 / hunters 1-2",
	"Team 1 slots: clients 3-4 / hunters 3-4",
	"Successful joins: 4/4",
	"Server-prepared players: $PreparedCount/4",
	"Team/slot/hunter mappings: $ValidMappings/4",
	"Primary ability activations: $SuccessfulActivations/20",
	"Input releases: $SuccessfulReleases/8",
	"Ability lifecycles: $(if ($SuccessfulActivations -eq 20 -and $SuccessfulReleases -eq 8) { '4/4' } else { 'FAIL' })",
	"Evidence: $($RunDirectory.FullName)"
)
if ($Failures.Count -gt 0) {
	$Summary += "Failures:"
	$Summary += $Failures | ForEach-Object { "- $_" }
}
$Summary | Set-Content -LiteralPath (Join-Path $RunDirectory.FullName "FourClientAbilitySummary.txt") -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Failures.Count -gt 0) {
	exit 1
}
