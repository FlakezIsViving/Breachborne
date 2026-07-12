param(
	[int]$BasePort = 7821,
	[int]$ConnectionSeconds = 45,
	[int]$PktLag = 100,
	[int]$PktLoss = 2,
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedNetworkImpairment"
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
Get-ChildItem -LiteralPath $OutputRoot -Directory | ForEach-Object { $ExistingRuns[$_.FullName] = $true }

$ExecArgument = "-ExecCmds=`"Net PktLag=$PktLag,Net PktLoss=$PktLoss`""
& (Join-Path $PSScriptRoot "TestPackagedFullRosterAbilitySmoke.ps1") `
	-BasePort $BasePort `
	-ConnectionSeconds $ConnectionSeconds `
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
if ($NewRuns.Count -ne 3) {
	$Failures += "Expected three new roster runs; observed $($NewRuns.Count)."
}

$ExpectedPairs = @("Hunters: 1, 2", "Hunters: 3, 4", "Hunters: 5, 6")
foreach ($ExpectedPair in $ExpectedPairs) {
	$MatchingRuns = @($NewRuns | Where-Object {
		$SummaryPath = Join-Path $_.FullName "AbilitySummary.txt"
		(Test-Path -LiteralPath $SummaryPath) -and
		((Get-Content -LiteralPath $SummaryPath -Raw) -match [regex]::Escape($ExpectedPair))
	})
	if ($MatchingRuns.Count -ne 1) {
		$Failures += "Expected one new passing run for '$ExpectedPair'; observed $($MatchingRuns.Count)."
	}
}

foreach ($Run in $NewRuns) {
	& (Join-Path $PSScriptRoot "ReviewInteractivePlaytest.ps1") -SessionDirectory $Run.FullName
	if ($LASTEXITCODE -ne 0) {
		$Failures += "$($Run.Name) failed packaged-log review."
	}
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
	"Breachborne packaged network-impairment smoke: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Hunters: 1-6",
	"Matches: $($NewRuns.Count)",
	"PktLag: $PktLag ms",
	"PktLoss: $PktLoss percent",
	"Evidence: $($NewRuns.FullName -join '; ')"
)
if ($Failures.Count -gt 0) {
	$Summary += "Failures:"
	$Summary += $Failures | ForEach-Object { "- $_" }
}

$SummaryPath = Join-Path $OutputRoot "NetworkImpairmentSummary.txt"
$Summary | Set-Content -LiteralPath $SummaryPath -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Failures.Count -gt 0) {
	exit 1
}
