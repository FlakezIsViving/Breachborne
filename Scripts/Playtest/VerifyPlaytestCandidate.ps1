param(
	[string]$BuildsRoot = "$PSScriptRoot\..\..\Builds",
	[string]$LogsRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedHandshake",
	[string]$AbilitySmokeRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedAbilitySmoke",
	[string]$NetworkImpairmentRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedNetworkImpairment",
	[string]$ReconnectRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedReconnect",
	[string]$UnrealPak = "C:\UnrealEngine-5.7.4-release\Engine\Binaries\Win64\UnrealPak.exe",
	[int]$ExpectedHudsonCueCount = 17
)

$ErrorActionPreference = "Stop"

function Require-Path([string]$Path, [string]$Description) {
	if (-not (Test-Path -LiteralPath $Path)) {
		throw "Missing $Description`: $Path"
	}
}

$ResolvedBuildsRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($BuildsRoot)
$ResolvedLogsRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($LogsRoot)
$ResolvedAbilitySmokeRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($AbilitySmokeRoot)
$ResolvedNetworkImpairmentRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($NetworkImpairmentRoot)
$ResolvedReconnectRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ReconnectRoot)
$ClientExe = Join-Path $ResolvedBuildsRoot "WindowsClient\Breachborne\Binaries\Win64\Breachborne.exe"
$ServerExe = Join-Path $ResolvedBuildsRoot "WindowsServer\Breachborne\Binaries\Win64\BreachborneServer.exe"
$ClientUtoc = Join-Path $ResolvedBuildsRoot "WindowsClient\Breachborne\Content\Paks\Breachborne-Windows.utoc"
$BuildSummary = Join-Path $ResolvedBuildsRoot "PlaytestBuildSummary.txt"
$VerificationSummary = Join-Path $ResolvedBuildsRoot "PlaytestCandidateVerification.txt"

Require-Path $ClientExe "packaged client executable"
Require-Path $ServerExe "packaged server executable"
Require-Path $ClientUtoc "client IoStore TOC"
Require-Path $BuildSummary "build summary"
Require-Path $UnrealPak "UnrealPak executable"
Require-Path $ResolvedLogsRoot "packaged handshake logs root"
Require-Path $ResolvedAbilitySmokeRoot "packaged ability-smoke logs root"
Require-Path $ResolvedNetworkImpairmentRoot "packaged network-impairment logs root"
Require-Path $ResolvedReconnectRoot "packaged reconnect logs root"

$BuildCompletedAt = (Get-Item -LiteralPath $BuildSummary).LastWriteTime
$ProjectRoot = Split-Path -Parent $PSScriptRoot | Split-Path -Parent
$InputExtensions = @('.h', '.cpp', '.cs', '.ini', '.uproject', '.uplugin', '.uasset', '.umap')
$CandidateInputs = @(
	Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'Source') -Recurse -File
	Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'Config') -Recurse -File
	Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'Content') -Recurse -File
	Get-ChildItem -LiteralPath (Join-Path $ProjectRoot 'Plugins') -Recurse -File |
		Where-Object { $_.FullName -notmatch '\\(Binaries|Intermediate|Saved)\\' }
	Get-Item -LiteralPath (Join-Path $ProjectRoot 'Breachborne.uproject')
) | Where-Object { $InputExtensions -contains $_.Extension.ToLowerInvariant() }
$NewestInput = $CandidateInputs | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if ($NewestInput -and $NewestInput.LastWriteTime -gt $BuildCompletedAt) {
	throw "Packaged candidate is stale. Newest input '$($NewestInput.FullName)' at $($NewestInput.LastWriteTime.ToString('o')) is newer than build summary $($BuildCompletedAt.ToString('o'))."
}
$AbilityEvidence = @()
foreach ($Pair in @("1, 2", "3, 4", "5, 6")) {
	$MatchingRun = Get-ChildItem -LiteralPath $ResolvedAbilitySmokeRoot -Directory |
		Where-Object {
			$SummaryPath = Join-Path $_.FullName "AbilitySummary.txt"
			if (-not (Test-Path -LiteralPath $SummaryPath)) { return $false }
			$Text = Get-Content -LiteralPath $SummaryPath -Raw
			return $Text -match 'ability smoke:\s+PASS' -and $Text -match "Hunters:\s+$([regex]::Escape($Pair))"
		} |
		Sort-Object LastWriteTime -Descending |
		Select-Object -First 1
	if (-not $MatchingRun) {
		throw "No passing packaged ability smoke found for hunters $Pair."
	}

	$AbilitySummaryPath = Join-Path $MatchingRun.FullName "AbilitySummary.txt"
	$ReviewSummaryPath = Join-Path $MatchingRun.FullName "ReviewSummary.txt"
	Require-Path $ReviewSummaryPath "ability-smoke log review for hunters $Pair"
	$ReviewText = Get-Content -LiteralPath $ReviewSummaryPath -Raw
	if ($ReviewText -notmatch 'log review:\s+PASS' -or
		$ReviewText -notmatch 'Critical findings:\s+0' -or
		$ReviewText -notmatch 'GameplayCue overflow findings:\s+0') {
		throw "Ability-smoke log review failed for hunters $Pair`: $ReviewSummaryPath"
	}
	if ((Get-Item -LiteralPath $AbilitySummaryPath).LastWriteTime -lt $BuildCompletedAt) {
		throw "Ability-smoke evidence for hunters $Pair predates the current package."
	}
	$AbilityEvidence += $MatchingRun.FullName
}

$NetworkEvidence = @()
foreach ($Pair in @("1, 2", "3, 4", "5, 6")) {
	$MatchingRun = Get-ChildItem -LiteralPath $ResolvedNetworkImpairmentRoot -Directory |
		Where-Object {
			$SummaryPath = Join-Path $_.FullName "AbilitySummary.txt"
			if (-not (Test-Path -LiteralPath $SummaryPath)) { return $false }
			$Text = Get-Content -LiteralPath $SummaryPath -Raw
			return $Text -match 'ability smoke:\s+PASS' -and
				$Text -match "Hunters:\s+$([regex]::Escape($Pair))" -and
				$Text -match 'Net PktLag=100' -and
				$Text -match 'Net PktLoss=2'
		} |
		Sort-Object LastWriteTime -Descending |
		Select-Object -First 1
	if (-not $MatchingRun) {
		throw "No passing 100 ms/2% loss packaged smoke found for hunters $Pair."
	}

	$AbilitySummaryPath = Join-Path $MatchingRun.FullName "AbilitySummary.txt"
	$ReviewSummaryPath = Join-Path $MatchingRun.FullName "ReviewSummary.txt"
	Require-Path $ReviewSummaryPath "network-impairment log review for hunters $Pair"
	$ReviewText = Get-Content -LiteralPath $ReviewSummaryPath -Raw
	if ($ReviewText -notmatch 'log review:\s+PASS' -or
		$ReviewText -notmatch 'Critical findings:\s+0' -or
		$ReviewText -notmatch 'GameplayCue overflow findings:\s+0') {
		throw "Network-impairment log review failed for hunters $Pair`: $ReviewSummaryPath"
	}
	if ((Get-Item -LiteralPath $AbilitySummaryPath).LastWriteTime -lt $BuildCompletedAt) {
		throw "Network-impairment evidence for hunters $Pair predates the current package."
	}
	foreach ($LogName in @("Server.log", "Client1.log", "Client2.log")) {
		$LogText = Get-Content -LiteralPath (Join-Path $MatchingRun.FullName $LogName) -Raw
		if ($LogText -notmatch 'LogNet: PktLag set to 100' -or
			$LogText -notmatch 'LogNet: PktLoss set to 2') {
			throw "Network impairment was not confirmed in $($MatchingRun.Name)/$LogName."
		}
	}
	$NetworkEvidence += $MatchingRun.FullName
}

$LatestReconnect = Get-ChildItem -LiteralPath $ResolvedReconnectRoot -Directory |
	Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName "ReconnectSummary.txt") } |
	Sort-Object LastWriteTime -Descending |
	Select-Object -First 1
if (-not $LatestReconnect) {
	throw "No packaged reconnect-attempt evidence found under $ResolvedReconnectRoot."
}
$ReconnectSummaryPath = Join-Path $LatestReconnect.FullName "ReconnectSummary.txt"
$ReconnectReviewPath = Join-Path $LatestReconnect.FullName "ReviewSummary.txt"
$ReconnectText = Get-Content -LiteralPath $ReconnectSummaryPath -Raw
Require-Path $ReconnectReviewPath "reconnect-attempt log review"
$ReconnectReviewText = Get-Content -LiteralPath $ReconnectReviewPath -Raw
if ($ReconnectText -notmatch 'reconnect attempt:\s+PASS_WITH_LIMITATION' -or
	$ReconnectText -notmatch 'Initial gameplay:\s+PASS' -or
	$ReconnectText -notmatch 'Transport reconnect:\s+PASS' -or
	$ReconnectText -notmatch 'Match-state restoration:\s+UNSUPPORTED') {
	throw "Latest packaged reconnect attempt has an unexpected result: $ReconnectSummaryPath"
}
if ($ReconnectReviewText -notmatch 'log review:\s+PASS' -or
	$ReconnectReviewText -notmatch 'Critical findings:\s+0' -or
	$ReconnectReviewText -notmatch 'GameplayCue overflow findings:\s+0') {
	throw "Reconnect-attempt log review failed: $ReconnectReviewPath"
}
if ((Get-Item -LiteralPath $ReconnectSummaryPath).LastWriteTime -lt $BuildCompletedAt) {
	throw "Reconnect-attempt evidence predates the current package."
}

$LatestHandshake = Get-ChildItem -LiteralPath $ResolvedLogsRoot -Directory |
	Sort-Object LastWriteTime -Descending |
	Select-Object -First 1
if (-not $LatestHandshake) {
	throw "No packaged handshake evidence found under $ResolvedLogsRoot"
}

$HandshakeSummary = Join-Path $LatestHandshake.FullName "Summary.txt"
Require-Path $HandshakeSummary "packaged handshake summary"
$HandshakeText = Get-Content -LiteralPath $HandshakeSummary -Raw
if ($HandshakeText -notmatch 'handshake:\s+PASS' -or $HandshakeText -notmatch 'Successful joins:\s+2') {
	throw "Latest packaged handshake did not pass 2/2`: $HandshakeSummary"
}

$CriticalPatterns = @(
	'Fatal error',
	'Ensure condition failed',
	'Assertion failed',
	'Accessed None',
	'no more RPCs',
	'OutdatedClient',
	'Network checksum mismatch',
	'LogNetPackageMap: Error',
	'Failed to load package',
	'Missing gameplay cue'
)
$CriticalMatches = @()
foreach ($Log in Get-ChildItem -LiteralPath $LatestHandshake.FullName -Filter '*.log') {
	$CriticalMatches += Select-String -LiteralPath $Log.FullName -Pattern $CriticalPatterns
}
if ($CriticalMatches.Count -gt 0) {
	$Details = ($CriticalMatches | ForEach-Object { "$($_.Path):$($_.LineNumber): $($_.Line.Trim())" }) -join [Environment]::NewLine
	throw "Critical packaged-log findings detected`:$([Environment]::NewLine)$Details"
}

$ArchiveListing = & $UnrealPak $ClientUtoc -List 2>&1
if ($LASTEXITCODE -ne 0) {
	throw "UnrealPak failed to list client IoStore (exit $LASTEXITCODE)."
}
$CueEntries = @($ArchiveListing | Select-String -Pattern 'Breachborne/Content/Hunters/Hudson/Cues/.+\.uasset')
$UniqueCueNames = @($CueEntries | ForEach-Object {
	if ($_.Line -match 'Cues/([^\"]+\.uasset)') { $Matches[1] }
} | Where-Object { $_ } | Sort-Object -Unique)
if ($UniqueCueNames.Count -ne $ExpectedHudsonCueCount) {
	throw "Expected $ExpectedHudsonCueCount cooked Hudson cues, found $($UniqueCueNames.Count)."
}
if (-not ($ArchiveListing | Select-String -Pattern 'Breachborne/Content/Maps/TestMap\.umap')) {
	throw "TestMap is missing from the client IoStore."
}

$Summary = @(
	"Breachborne playtest candidate verification: PASS",
	"Timestamp: $(Get-Date -Format o)",
	"Client: $ClientExe",
	"Server: $ServerExe",
	"Cooked Hudson cues: $($UniqueCueNames.Count)/$ExpectedHudsonCueCount",
	"Cooked map: /Game/Maps/TestMap",
	"Packaged handshake: PASS (2/2)",
	"Handshake evidence: $($LatestHandshake.FullName)",
	"Packaged ability smoke: PASS (6/6 hunters, 3 matches)",
	"Ability-smoke evidence: $($AbilityEvidence -join '; ')",
	"Network impairment: PASS (6/6 hunters, 3 matches, 100 ms lag, 2% loss)",
	"Network-impairment evidence: $($NetworkEvidence -join '; ')",
	"Reconnect attempt: TRANSPORT PASS; match-state restoration UNSUPPORTED",
	"Reconnect evidence: $($LatestReconnect.FullName)",
	"Critical log findings: 0"
)
$Summary | Set-Content -LiteralPath $VerificationSummary -Encoding UTF8
Write-Host ($Summary -join [Environment]::NewLine)
Write-Host "Verification summary: $VerificationSummary"
