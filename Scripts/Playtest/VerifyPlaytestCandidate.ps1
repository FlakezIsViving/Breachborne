param(
	[string]$BuildsRoot = "$PSScriptRoot\..\..\Builds",
	[string]$LogsRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedHandshake",
	[string]$AbilitySmokeRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedAbilitySmoke",
	[string]$MatchResetRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedMatchReset",
	[string]$NetworkImpairmentRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedNetworkImpairment",
	[string]$ReconnectRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedReconnect",
	[string]$DeathWispRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedDeathWisp",
	[string]$WispRulesRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedWispRules",
	[string]$HitSmokeRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedHitSmoke",
	[string]$OutcomeSmokeRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedOutcomeSmoke",
	[string]$OutcomeNetworkImpairmentRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedOutcomeNetworkImpairment",
	[string]$FourClientRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedFourClientHandshake",
	[string]$FourClientAbilityRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedFourClientAbilitySmoke",
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
$ResolvedMatchResetRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($MatchResetRoot)
$ResolvedNetworkImpairmentRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($NetworkImpairmentRoot)
$ResolvedReconnectRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ReconnectRoot)
$ResolvedDeathWispRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($DeathWispRoot)
$ResolvedWispRulesRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($WispRulesRoot)
$ResolvedHitSmokeRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($HitSmokeRoot)
$ResolvedOutcomeSmokeRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutcomeSmokeRoot)
$ResolvedOutcomeNetworkImpairmentRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutcomeNetworkImpairmentRoot)
$ResolvedFourClientRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($FourClientRoot)
$ResolvedFourClientAbilityRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($FourClientAbilityRoot)
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
Require-Path $ResolvedMatchResetRoot "packaged match-reset logs root"
Require-Path $ResolvedNetworkImpairmentRoot "packaged network-impairment logs root"
Require-Path $ResolvedReconnectRoot "packaged reconnect logs root"
Require-Path $ResolvedDeathWispRoot "packaged death/wisp logs root"
Require-Path $ResolvedWispRulesRoot "packaged wisp-rules logs root"
Require-Path $ResolvedHitSmokeRoot "packaged LMB hit-smoke logs root"
Require-Path $ResolvedOutcomeSmokeRoot "packaged non-LMB outcome-smoke logs root"
Require-Path $ResolvedOutcomeNetworkImpairmentRoot "packaged impaired non-LMB outcome-smoke logs root"
Require-Path $ResolvedFourClientRoot "packaged four-client handshake logs root"
Require-Path $ResolvedFourClientAbilityRoot "packaged four-client 2v2 ability-smoke logs root"

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

$LatestMatchReset = Get-ChildItem -LiteralPath $ResolvedMatchResetRoot -Directory |
	Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName "MatchResetSummary.txt") } |
	Sort-Object LastWriteTime -Descending |
	Select-Object -First 1
if (-not $LatestMatchReset) {
	throw "No packaged match-reset evidence found under $ResolvedMatchResetRoot."
}
$MatchResetSummaryPath = Join-Path $LatestMatchReset.FullName "MatchResetSummary.txt"
$MatchResetReviewPath = Join-Path $LatestMatchReset.FullName "ReviewSummary.txt"
$MatchResetText = Get-Content -LiteralPath $MatchResetSummaryPath -Raw
Require-Path $MatchResetReviewPath "match-reset log review"
$MatchResetReviewText = Get-Content -LiteralPath $MatchResetReviewPath -Raw
foreach ($RequiredResult in @(
	'packaged match-reset smoke:\s+PASS',
	'Clean server resets:\s+2/2',
	'Ghost post-reset activations:\s+5/5')) {
	if ($MatchResetText -notmatch $RequiredResult) {
		throw "Match-reset evidence is incomplete: $MatchResetSummaryPath"
	}
}
if ($MatchResetReviewText -notmatch 'log review:\s+PASS' -or
	$MatchResetReviewText -notmatch 'Critical findings:\s+0' -or
	$MatchResetReviewText -notmatch 'GameplayCue overflow findings:\s+0') {
	throw "Match-reset log review failed: $MatchResetReviewPath"
}
if ((Get-Item -LiteralPath $MatchResetSummaryPath).LastWriteTime -lt $BuildCompletedAt) {
	throw "Match-reset evidence predates the current package."
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
if ($ReconnectText -notmatch 'reconnect attempt:\s+PASS' -or
	$ReconnectText -notmatch 'Initial gameplay:\s+PASS' -or
	$ReconnectText -notmatch 'Transport reconnect:\s+PASS' -or
	$ReconnectText -notmatch 'Match-state restoration:\s+PASS') {
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

$DeathWispEvidence = @()
foreach ($Direction in @("1 kills 2", "2 kills 1")) {
	$MatchingRun = Get-ChildItem -LiteralPath $ResolvedDeathWispRoot -Directory |
		Where-Object {
			$SummaryPath = Join-Path $_.FullName "DeathWispSummary.txt"
			if (-not (Test-Path -LiteralPath $SummaryPath)) { return $false }
			$Text = Get-Content -LiteralPath $SummaryPath -Raw
			return $Text -match 'death/wisp smoke:\s+PASS' -and
				$Text -match "Hunters:\s+$([regex]::Escape($Direction))"
		} |
		Sort-Object LastWriteTime -Descending |
		Select-Object -First 1
	if (-not $MatchingRun) {
		throw "No passing packaged death/wisp evidence found for direction $Direction."
	}

	$DeathWispSummaryPath = Join-Path $MatchingRun.FullName "DeathWispSummary.txt"
	$DeathWispReviewPath = Join-Path $MatchingRun.FullName "ReviewSummary.txt"
	$DeathWispText = Get-Content -LiteralPath $DeathWispSummaryPath -Raw
	Require-Path $DeathWispReviewPath "death/wisp log review for direction $Direction"
	$DeathWispReviewText = Get-Content -LiteralPath $DeathWispReviewPath -Raw
	foreach ($RequiredResult in @(
		'Lethal GAS damage:\s+PASS',
		'Server wisp spawn/possession:\s+PASS',
		'Owned actor/transient state cleanup:\s+PASS',
		'Victim client wisp observation:\s+PASS',
		'Healing-driven revive:\s+PASS',
		'Server passive restart:\s+PASS',
		'Victim client hunter repossession:\s+PASS')) {
		if ($DeathWispText -notmatch $RequiredResult) {
			throw "Death/wisp evidence is incomplete for direction $Direction`: $DeathWispSummaryPath"
		}
	}
	if ($DeathWispReviewText -notmatch 'log review:\s+PASS' -or
		$DeathWispReviewText -notmatch 'Critical findings:\s+0' -or
		$DeathWispReviewText -notmatch 'GameplayCue overflow findings:\s+0') {
		throw "Death/wisp log review failed for direction $Direction`: $DeathWispReviewPath"
	}
	if ((Get-Item -LiteralPath $DeathWispSummaryPath).LastWriteTime -lt $BuildCompletedAt) {
		throw "Death/wisp evidence for direction $Direction predates the current package."
	}
	$DeathWispEvidence += $MatchingRun.FullName
}

$LatestWispRules = Get-ChildItem -LiteralPath $ResolvedWispRulesRoot -Directory |
	Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName "WispRulesSummary.txt") } |
	Sort-Object LastWriteTime -Descending |
	Select-Object -First 1
if (-not $LatestWispRules) {
	throw "No packaged wisp-rules evidence found under $ResolvedWispRulesRoot."
}
$WispRulesSummaryPath = Join-Path $LatestWispRules.FullName "WispRulesSummary.txt"
$WispRulesReviewPath = Join-Path $LatestWispRules.FullName "ReviewSummary.txt"
$WispRulesText = Get-Content -LiteralPath $WispRulesSummaryPath -Raw
Require-Path $WispRulesReviewPath "wisp-rules log review"
$WispRulesReviewText = Get-Content -LiteralPath $WispRulesReviewPath -Raw
foreach ($RequiredResult in @(
	'packaged wisp-rules smoke:\s+PASS',
	'Death/wisp possession:\s+PASS',
	'Wisp rule natural:\s+PASS',
	'Wisp rule ally:\s+PASS',
	'Wisp rule ally_enemy:\s+PASS',
	'Wisp rule enemy:\s+PASS',
	'Wisp rule healing:\s+PASS',
	'Wisp rule healing_enemy:\s+PASS',
	'Wisp rule carried_enemy:\s+PASS',
	'Wisp rule eluna_pickup:\s+PASS',
	'Wisp rule eluna_cc_drop:\s+PASS',
	'Wisp rule eluna_r_revive:\s+PASS',
	'Eluna R revive/re-possession:\s+PASS')) {
	if ($WispRulesText -notmatch $RequiredResult) {
		throw "Wisp-rules evidence is incomplete: $WispRulesSummaryPath"
	}
}
if ($WispRulesReviewText -notmatch 'log review:\s+PASS' -or
	$WispRulesReviewText -notmatch 'Critical findings:\s+0' -or
	$WispRulesReviewText -notmatch 'GameplayCue overflow findings:\s+0') {
	throw "Wisp-rules log review failed: $WispRulesReviewPath"
}
if ((Get-Item -LiteralPath $WispRulesSummaryPath).LastWriteTime -lt $BuildCompletedAt) {
	throw "Wisp-rules evidence predates the current package."
}

$HitEvidence = @()
foreach ($Pair in @("1, 2", "3, 4", "5, 6")) {
	$MatchingRun = Get-ChildItem -LiteralPath $ResolvedHitSmokeRoot -Directory |
		Where-Object {
			$SummaryPath = Join-Path $_.FullName "HitSummary.txt"
			if (-not (Test-Path -LiteralPath $SummaryPath)) { return $false }
			$Text = Get-Content -LiteralPath $SummaryPath -Raw
			return $Text -match 'LMB hit smoke:\s+PASS' -and
				$Text -match "Hunters:\s+$([regex]::Escape($Pair))" -and
				$Text -match 'Authoritative LMB damage reports:\s+2/2'
		} |
		Sort-Object LastWriteTime -Descending |
		Select-Object -First 1
	if (-not $MatchingRun) {
		throw "No passing packaged LMB hit smoke found for hunters $Pair."
	}

	$HitSummaryPath = Join-Path $MatchingRun.FullName "HitSummary.txt"
	$HitReviewPath = Join-Path $MatchingRun.FullName "ReviewSummary.txt"
	Require-Path $HitReviewPath "LMB hit-smoke log review for hunters $Pair"
	$HitReviewText = Get-Content -LiteralPath $HitReviewPath -Raw
	if ($HitReviewText -notmatch 'log review:\s+PASS' -or
		$HitReviewText -notmatch 'Critical findings:\s+0' -or
		$HitReviewText -notmatch 'GameplayCue overflow findings:\s+0') {
		throw "LMB hit-smoke log review failed for hunters $Pair`: $HitReviewPath"
	}
	if ((Get-Item -LiteralPath $HitSummaryPath).LastWriteTime -lt $BuildCompletedAt) {
		throw "LMB hit-smoke evidence for hunters $Pair predates the current package."
	}
	$HitEvidence += $MatchingRun.FullName
}

$OutcomeEvidence = @()
for ($HunterID = 1; $HunterID -le 6; $HunterID++) {
	$MatchingRun = Get-ChildItem -LiteralPath $ResolvedOutcomeSmokeRoot -Directory |
		Where-Object {
			$SummaryPath = Join-Path $_.FullName "OutcomeSummary.txt"
			if (-not (Test-Path -LiteralPath $SummaryPath)) { return $false }
			$Text = Get-Content -LiteralPath $SummaryPath -Raw
			return $Text -match 'non-LMB outcome smoke:\s+PASS' -and
				$Text -match "Attacker hunter:\s+$HunterID" -and
				$Text -match 'Authoritative outcome contracts:\s+4/4'
		} |
		Sort-Object LastWriteTime -Descending |
		Select-Object -First 1
	if (-not $MatchingRun) {
		throw "No passing packaged non-LMB outcome smoke found for hunter $HunterID."
	}

	$OutcomeSummaryPath = Join-Path $MatchingRun.FullName "OutcomeSummary.txt"
	$OutcomeReviewPath = Join-Path $MatchingRun.FullName "ReviewSummary.txt"
	Require-Path $OutcomeReviewPath "non-LMB outcome-smoke log review for hunter $HunterID"
	$OutcomeReviewText = Get-Content -LiteralPath $OutcomeReviewPath -Raw
	if ($OutcomeReviewText -notmatch 'log review:\s+PASS' -or
		$OutcomeReviewText -notmatch 'Critical findings:\s+0' -or
		$OutcomeReviewText -notmatch 'GameplayCue overflow findings:\s+0') {
		throw "Non-LMB outcome-smoke log review failed for hunter $HunterID`: $OutcomeReviewPath"
	}
	if ((Get-Item -LiteralPath $OutcomeSummaryPath).LastWriteTime -lt $BuildCompletedAt) {
		throw "Non-LMB outcome-smoke evidence for hunter $HunterID predates the current package."
	}
	$OutcomeEvidence += $MatchingRun.FullName
}

$ImpairedOutcomeEvidence = @()
for ($HunterID = 1; $HunterID -le 6; $HunterID++) {
	$MatchingRun = Get-ChildItem -LiteralPath $ResolvedOutcomeNetworkImpairmentRoot -Directory |
		Where-Object {
			$SummaryPath = Join-Path $_.FullName "OutcomeSummary.txt"
			if (-not (Test-Path -LiteralPath $SummaryPath)) { return $false }
			$Text = Get-Content -LiteralPath $SummaryPath -Raw
			return $Text -match 'non-LMB outcome smoke:\s+PASS' -and
				$Text -match "Attacker hunter:\s+$HunterID" -and
				$Text -match 'Authoritative outcome contracts:\s+4/4' -and
				$Text -match 'Server extra args:.*PktLag=100.*PktLoss=2' -and
				$Text -match 'Client extra args:.*PktLag=100.*PktLoss=2'
		} |
		Sort-Object LastWriteTime -Descending |
		Select-Object -First 1
	if (-not $MatchingRun) {
		throw "No passing 100 ms/2% loss packaged non-LMB outcome smoke found for hunter $HunterID."
	}

	$OutcomeSummaryPath = Join-Path $MatchingRun.FullName "OutcomeSummary.txt"
	$OutcomeReviewPath = Join-Path $MatchingRun.FullName "ReviewSummary.txt"
	Require-Path $OutcomeReviewPath "impaired non-LMB outcome-smoke log review for hunter $HunterID"
	$OutcomeReviewText = Get-Content -LiteralPath $OutcomeReviewPath -Raw
	if ($OutcomeReviewText -notmatch 'log review:\s+PASS' -or
		$OutcomeReviewText -notmatch 'Critical findings:\s+0' -or
		$OutcomeReviewText -notmatch 'GameplayCue overflow findings:\s+0') {
		throw "Impaired non-LMB outcome-smoke log review failed for hunter $HunterID`: $OutcomeReviewPath"
	}
	if ((Get-Item -LiteralPath $OutcomeSummaryPath).LastWriteTime -lt $BuildCompletedAt) {
		throw "Impaired non-LMB outcome-smoke evidence for hunter $HunterID predates the current package."
	}
	foreach ($LogName in @("Server.log", "Client1.log", "Client2.log")) {
		$LogText = Get-Content -LiteralPath (Join-Path $MatchingRun.FullName $LogName) -Raw
		if ($LogText -notmatch 'LogNet: PktLag set to 100' -or
			$LogText -notmatch 'LogNet: PktLoss set to 2') {
			throw "Outcome impairment was not confirmed in $($MatchingRun.Name)/$LogName."
		}
	}
	$ImpairedOutcomeEvidence += $MatchingRun.FullName
}

$LatestFourClient = Get-ChildItem -LiteralPath $ResolvedFourClientRoot -Directory |
	Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName "FourClientSummary.txt") } |
	Sort-Object LastWriteTime -Descending |
	Select-Object -First 1
if (-not $LatestFourClient) {
	throw "No packaged four-client handshake evidence found under $ResolvedFourClientRoot."
}
$FourClientSummaryPath = Join-Path $LatestFourClient.FullName "FourClientSummary.txt"
$FourClientReviewPath = Join-Path $LatestFourClient.FullName "ReviewSummary.txt"
$FourClientText = Get-Content -LiteralPath $FourClientSummaryPath -Raw
Require-Path $FourClientReviewPath "four-client handshake log review"
$FourClientReviewText = Get-Content -LiteralPath $FourClientReviewPath -Raw
if ($FourClientText -notmatch 'four-client handshake:\s+PASS' -or
	$FourClientText -notmatch 'Successful joins:\s+4/4') {
	throw "Latest packaged four-client handshake did not pass 4/4: $FourClientSummaryPath"
}
if ($FourClientReviewText -notmatch 'log review:\s+PASS' -or
	$FourClientReviewText -notmatch 'Critical findings:\s+0' -or
	$FourClientReviewText -notmatch 'GameplayCue overflow findings:\s+0') {
	throw "Four-client handshake log review failed: $FourClientReviewPath"
}
if ((Get-Item -LiteralPath $FourClientSummaryPath).LastWriteTime -lt $BuildCompletedAt) {
	throw "Four-client handshake evidence predates the current package."
}

$LatestFourClientAbility = Get-ChildItem -LiteralPath $ResolvedFourClientAbilityRoot -Directory |
	Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName "FourClientAbilitySummary.txt") } |
	Sort-Object LastWriteTime -Descending |
	Select-Object -First 1
if (-not $LatestFourClientAbility) {
	throw "No packaged four-client 2v2 ability-smoke evidence found under $ResolvedFourClientAbilityRoot."
}
$FourClientAbilitySummaryPath = Join-Path $LatestFourClientAbility.FullName "FourClientAbilitySummary.txt"
$FourClientAbilityReviewPath = Join-Path $LatestFourClientAbility.FullName "ReviewSummary.txt"
$FourClientAbilityText = Get-Content -LiteralPath $FourClientAbilitySummaryPath -Raw
Require-Path $FourClientAbilityReviewPath "four-client 2v2 ability-smoke log review"
$FourClientAbilityReviewText = Get-Content -LiteralPath $FourClientAbilityReviewPath -Raw
foreach ($RequiredResult in @(
	'four-client 2v2 ability smoke:\s+PASS',
	'Successful joins:\s+4/4',
	'Server-prepared players:\s+4/4',
	'Team/slot/hunter mappings:\s+4/4',
	'Primary ability activations:\s+20/20',
	'Input releases:\s+8/8',
	'Ability lifecycles:\s+4/4')) {
	if ($FourClientAbilityText -notmatch $RequiredResult) {
		throw "Four-client 2v2 ability-smoke evidence is incomplete: $FourClientAbilitySummaryPath"
	}
}
if ($FourClientAbilityReviewText -notmatch 'log review:\s+PASS' -or
	$FourClientAbilityReviewText -notmatch 'Critical findings:\s+0' -or
	$FourClientAbilityReviewText -notmatch 'GameplayCue overflow findings:\s+0') {
	throw "Four-client 2v2 ability-smoke log review failed: $FourClientAbilityReviewPath"
}
if ((Get-Item -LiteralPath $FourClientAbilitySummaryPath).LastWriteTime -lt $BuildCompletedAt) {
	throw "Four-client 2v2 ability-smoke evidence predates the current package."
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
	"Persistent match reset: PASS (Dead/Wisp/cooldown cleanup; Ghost activations 5/5)",
	"Match-reset evidence: $($LatestMatchReset.FullName)",
	"Network impairment: PASS (6/6 hunters, 3 matches, 100 ms lag, 2% loss)",
	"Network-impairment evidence: $($NetworkEvidence -join '; ')",
	"Reconnect attempt: PASS (transport plus same-hunter match-state restoration)",
	"Reconnect evidence: $($LatestReconnect.FullName)",
	"Death/wisp/revive: PASS (both victim directions; GAS death, cleanup, wisp possession, healing revive, passive restart, hunter repossession)",
	"Death/wisp evidence: $($DeathWispEvidence -join '; ')",
	"Wisp lifecycle rules: PASS (10/10: natural, ally, enemy, contest, healing, carried exception, Eluna pickup/CC drop, Eluna R revive)",
	"Wisp-rules evidence: $($LatestWispRules.FullName)",
	"Authoritative LMB hits: PASS (6/6 hunters, 3 matches)",
	"LMB hit evidence: $($HitEvidence -join '; ')",
	"Authoritative non-LMB outcomes: PASS (24/24 contracts, 6 matches)",
	"Non-LMB outcome evidence: $($OutcomeEvidence -join '; ')",
	"Impaired authoritative non-LMB outcomes: PASS (24/24 contracts, 6 matches, 100 ms lag, 2% loss)",
	"Impaired non-LMB outcome evidence: $($ImpairedOutcomeEvidence -join '; ')",
	"Four-client transport handshake: PASS (4/4 joins)",
	"Four-client evidence: $($LatestFourClient.FullName)",
	"Four-client 2v2 ability smoke: PASS (4/4 mappings and lifecycles; 20/20 primary activations; 8/8 releases)",
	"Four-client 2v2 evidence: $($LatestFourClientAbility.FullName)",
	"Critical log findings: 0"
)
$Summary | Set-Content -LiteralPath $VerificationSummary -Encoding UTF8
Write-Host ($Summary -join [Environment]::NewLine)
Write-Host "Verification summary: $VerificationSummary"
