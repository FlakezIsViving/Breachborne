param(
	[string]$SessionDirectory = "",
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\InteractivePlaytest"
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($SessionDirectory)) {
	$LatestPath = Join-Path $OutputRoot "LatestSession.txt"
	if (-not (Test-Path -LiteralPath $LatestPath)) {
		throw "No interactive session record exists at $LatestPath"
	}
	$SessionDirectory = (Get-Content -LiteralPath $LatestPath -Raw).Trim()
}

$ResolvedSession = (Resolve-Path -LiteralPath $SessionDirectory).ProviderPath
$SessionPath = Join-Path $ResolvedSession "Session.json"
$Session = if (Test-Path -LiteralPath $SessionPath) {
	Get-Content -LiteralPath $SessionPath -Raw | ConvertFrom-Json
} else { $null }
$Logs = Get-ChildItem -LiteralPath $ResolvedSession -Filter "*.log" -File
if (-not $Logs) {
	throw "No logs found in $ResolvedSession"
}

$CriticalPattern = "Fatal error|Critical error|Ensure condition failed|Assertion failed|OutdatedClient|Network Failure|Travel Failure|Connection TIMED OUT"
$CueOverflowPattern = "GameplayCue.*exceed|GameplayCue.*overflow|Too many GameplayCue|dropped.*GameplayCue|GameplayCue.*no more RPCs are allowed|no more RPCs are allowed.*GameplayCue"
$CriticalFindings = @($Logs | Select-String -Pattern $CriticalPattern)
$CueOverflowFindings = @($Logs | Select-String -Pattern $CueOverflowPattern)
$ServerLog = $Logs | Where-Object Name -eq "Server.log" | Select-Object -First 1
$JoinCount = if ($ServerLog) {
	(Select-String -Path $ServerLog.FullName -Pattern "Join succeeded:" | Measure-Object).Count
} else { 0 }
$ExpectedJoins = if ($Session -and $null -ne $Session.ClientCount) { [int]$Session.ClientCount } else { 0 }
$JoinCheckPassed = $ExpectedJoins -le 0 -or $JoinCount -ge $ExpectedJoins

$EvidenceLines = @($Logs | Select-String -Pattern "BB_NETFLOW|BB_LOBBY|BB_MATCH|WispSpawn|GameplayCue" |
	ForEach-Object { "$($_.Path):$($_.LineNumber):$($_.Line.Trim())" })
$EvidencePath = Join-Path $ResolvedSession "RelevantEvents.txt"
$EvidenceLines | Set-Content -LiteralPath $EvidencePath -Encoding UTF8

$Result = if ($CriticalFindings.Count -eq 0 -and $CueOverflowFindings.Count -eq 0 -and $JoinCheckPassed) { "PASS" } else { "REVIEW" }
$SessionLabel = if ($Session -and -not [string]::IsNullOrWhiteSpace([string]$Session.SessionLabel)) {
	[string]$Session.SessionLabel
} else { "unlabeled" }
$ServerArgsText = if ($Session -and $Session.ServerExtraArgs) { @($Session.ServerExtraArgs) -join " " } else { "none" }
$ClientArgsText = if ($Session -and $Session.ClientExtraArgs) { @($Session.ClientExtraArgs) -join " " } else { "none" }
$JoinText = if ($ExpectedJoins -gt 0) { "$JoinCount/$ExpectedJoins" } else { [string]$JoinCount }
$Summary = @(
	"Breachborne interactive packaged log review: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Session: $ResolvedSession",
	"Session label: $SessionLabel",
	"Logs reviewed: $($Logs.Count)",
	"Server joins: $JoinText",
	"Server extra args: $ServerArgsText",
	"Client extra args: $ClientArgsText",
	"Critical findings: $($CriticalFindings.Count)",
	"GameplayCue overflow findings: $($CueOverflowFindings.Count)",
	"Relevant events: $EvidencePath"
)
if (-not $JoinCheckPassed) {
	$Summary += "Join check failed: expected at least $ExpectedJoins clients, observed $JoinCount."
}
if ($CriticalFindings.Count -gt 0) {
	$Summary += "Critical matches:"
	$Summary += $CriticalFindings | ForEach-Object { "- $($_.Path):$($_.LineNumber): $($_.Line.Trim())" }
}
if ($CueOverflowFindings.Count -gt 0) {
	$Summary += "GameplayCue matches:"
	$Summary += $CueOverflowFindings | ForEach-Object { "- $($_.Path):$($_.LineNumber): $($_.Line.Trim())" }
}

$Summary | Set-Content -LiteralPath (Join-Path $ResolvedSession "ReviewSummary.txt") -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Result -ne "PASS") {
	exit 1
}

$global:LASTEXITCODE = 0
