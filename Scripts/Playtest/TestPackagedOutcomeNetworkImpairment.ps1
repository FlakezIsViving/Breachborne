param(
	[int]$BasePort = 7961,
	[int]$ConnectionSeconds = 60,
	[int]$PktLag = 100,
	[int]$PktLoss = 2,
	[string]$ClientBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsClient",
	[string]$ServerBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsServer",
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedOutcomeNetworkImpairment"
)

$ErrorActionPreference = "Stop"

if ($PktLag -lt 0) {
	throw "PktLag cannot be negative."
}
if ($PktLoss -lt 0 -or $PktLoss -gt 100) {
	throw "PktLoss must be between 0 and 100."
}

New-Item -ItemType Directory -Force -Path $OutputRoot | Out-Null
$ExistingRuns = @{}
Get-ChildItem -LiteralPath $OutputRoot -Directory | ForEach-Object {
	$ExistingRuns[$_.FullName] = $true
}

$ExecArgument = "-ExecCmds=`"Net PktLag=$PktLag,Net PktLoss=$PktLoss`""
& (Join-Path $PSScriptRoot "TestPackagedFullRosterOutcomeSmoke.ps1") `
	-BasePort $BasePort `
	-ConnectionSeconds $ConnectionSeconds `
	-ClientBuildRoot $ClientBuildRoot `
	-ServerBuildRoot $ServerBuildRoot `
	-OutputRoot $OutputRoot `
	-ServerExtraArgs @($ExecArgument) `
	-ClientExtraArgs @($ExecArgument)
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$NewRuns = @(Get-ChildItem -LiteralPath $OutputRoot -Directory |
	Where-Object { -not $ExistingRuns.ContainsKey($_.FullName) } |
	Sort-Object Name)
$Failures = @()
if ($NewRuns.Count -ne 6) {
	$Failures += "Expected six new hunter outcome runs; observed $($NewRuns.Count)."
}

for ($HunterID = 1; $HunterID -le 6; $HunterID++) {
	$MatchingRuns = @($NewRuns | Where-Object {
		$SummaryPath = Join-Path $_.FullName "OutcomeSummary.txt"
		if (-not (Test-Path -LiteralPath $SummaryPath)) { return $false }
		$SummaryText = Get-Content -LiteralPath $SummaryPath -Raw
		return $SummaryText -match 'non-LMB outcome smoke:\s+PASS' -and
			$SummaryText -match "Attacker hunter:\s+$HunterID" -and
			$SummaryText -match 'Authoritative outcome contracts:\s+4/4'
	})
	if ($MatchingRuns.Count -ne 1) {
		$Failures += "Expected one new passing run for hunter $HunterID; observed $($MatchingRuns.Count)."
	}
}

foreach ($Run in $NewRuns) {
	foreach ($LogName in @("Server.log", "Client1.log", "Client2.log")) {
		$LogPath = Join-Path $Run.FullName $LogName
		if (-not (Test-Path -LiteralPath $LogPath)) {
			$Failures += "$($Run.Name) is missing $LogName."
			continue
		}
		$LogText = Get-Content -LiteralPath $LogPath -Raw
		if ($LogText -notmatch "LogNet: PktLag set to $PktLag") {
			$Failures += "$($Run.Name)/$LogName did not confirm PktLag=$PktLag."
		}
		if ($LogText -notmatch "LogNet: PktLoss set to $PktLoss") {
			$Failures += "$($Run.Name)/$LogName did not confirm PktLoss=$PktLoss."
		}
	}
}

$Result = if ($Failures.Count -eq 0) { "PASS" } else { "FAIL" }
$Summary = @(
	"Breachborne packaged non-LMB outcome network-impairment smoke: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Hunters: 1-6",
	"Authoritative outcome contracts: 24/24",
	"Matches: $($NewRuns.Count)",
	"PktLag: $PktLag ms",
	"PktLoss: $PktLoss percent",
	"Evidence: $($NewRuns.FullName -join '; ')"
)
if ($Failures.Count -gt 0) {
	$Summary += "Failures:"
	$Summary += $Failures | ForEach-Object { "- $_" }
}

$SummaryPath = Join-Path $OutputRoot "OutcomeNetworkImpairmentSummary.txt"
$Summary | Set-Content -LiteralPath $SummaryPath -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Failures.Count -gt 0) {
	exit 1
}
