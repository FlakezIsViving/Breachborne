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
$Logs = Get-ChildItem -LiteralPath $ResolvedSession -Filter "*.log" -File
if (-not $Logs) {
	throw "No logs found in $ResolvedSession"
}

$CriticalPattern = "Fatal error|Critical error|Ensure condition failed|Assertion failed|OutdatedClient|Network Failure|Travel Failure|Connection TIMED OUT"
$CueOverflowPattern = "GameplayCue.*exceed|GameplayCue.*overflow|Too many GameplayCue|dropped.*GameplayCue"
$CriticalFindings = @($Logs | Select-String -Pattern $CriticalPattern)
$CueOverflowFindings = @($Logs | Select-String -Pattern $CueOverflowPattern)
$ServerLog = $Logs | Where-Object Name -eq "Server.log" | Select-Object -First 1
$JoinCount = if ($ServerLog) {
	(Select-String -Path $ServerLog.FullName -Pattern "Join succeeded:" | Measure-Object).Count
} else { 0 }

$EvidenceLines = @($Logs | Select-String -Pattern "BB_NETFLOW|BB_LOBBY|BB_MATCH|WispSpawn|GameplayCue" |
	ForEach-Object { "$($_.Path):$($_.LineNumber):$($_.Line.Trim())" })
$EvidencePath = Join-Path $ResolvedSession "RelevantEvents.txt"
$EvidenceLines | Set-Content -LiteralPath $EvidencePath -Encoding UTF8

$Result = if ($CriticalFindings.Count -eq 0 -and $CueOverflowFindings.Count -eq 0) { "PASS" } else { "REVIEW" }
$Summary = @(
	"Breachborne interactive packaged log review: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Session: $ResolvedSession",
	"Logs reviewed: $($Logs.Count)",
	"Server joins: $JoinCount",
	"Critical findings: $($CriticalFindings.Count)",
	"GameplayCue overflow findings: $($CueOverflowFindings.Count)",
	"Relevant events: $EvidencePath"
)
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
