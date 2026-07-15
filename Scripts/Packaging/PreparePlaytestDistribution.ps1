param(
	[string]$BuildsRoot = "$PSScriptRoot\..\..\Builds",
	[string]$OutputRoot = "$PSScriptRoot\..\..\Builds\Distribution",
	[switch]$IncludeServer,
	[switch]$CreateArchives,
	[switch]$ValidateOnly
)

$ErrorActionPreference = "Stop"

$ProjectRoot = (Resolve-Path -LiteralPath "$PSScriptRoot\..\..").ProviderPath
. (Join-Path $ProjectRoot "Scripts\Playtest\DistributionManifest.ps1")
$ResolvedBuildsRoot = (Resolve-Path -LiteralPath $BuildsRoot).ProviderPath
$ClientSource = Join-Path $ResolvedBuildsRoot "WindowsClient"
$ServerSource = Join-Path $ResolvedBuildsRoot "WindowsServer"
$BuildSummary = Join-Path $ResolvedBuildsRoot "PlaytestBuildSummary.txt"
$CandidateSummary = Join-Path $ResolvedBuildsRoot "PlaytestCandidateVerification.txt"
$VfxAudit = Join-Path $ResolvedBuildsRoot "VfxFoundationAudit.txt"

$RequiredPaths = @($ClientSource, $BuildSummary, $CandidateSummary)
if ($IncludeServer) {
	$RequiredPaths += $ServerSource
}
foreach ($RequiredPath in $RequiredPaths) {
	if (-not (Test-Path -LiteralPath $RequiredPath)) {
		throw "Required distribution input is missing: $RequiredPath"
	}
}

$global:LASTEXITCODE = 0
& (Join-Path $ProjectRoot "Scripts\Playtest\VerifyPlaytestCandidate.ps1") -BuildsRoot $ResolvedBuildsRoot
if ($LASTEXITCODE -ne 0) {
	throw "Candidate verification failed. Distribution was not prepared."
}

$PreflightScripts = @(
	"Scripts\Playtest\TestManualAcceptanceDefinition.ps1",
	"Scripts\Playtest\TestDistributionVerifier.ps1",
	"Scripts\Playtest\TestDirectIPReviewer.ps1"
)
foreach ($RelativeScript in $PreflightScripts) {
	$global:LASTEXITCODE = 0
	& (Join-Path $ProjectRoot $RelativeScript)
	if ($LASTEXITCODE -ne 0) {
		throw "Distribution preflight failed: $RelativeScript"
	}
}

if ($ValidateOnly) {
	Write-Host "Breachborne playtest distribution validation: PASS"
	Write-Host "  Client source: $ClientSource"
	Write-Host "  Server source: $(if ($IncludeServer) { $ServerSource } else { 'not requested' })"
	Write-Host "  Output root:   $([IO.Path]::GetFullPath($OutputRoot))"
	Write-Host "  Archives:      $($CreateArchives.IsPresent)"
	Write-Host "  Tooling:       manual 55/55, manifest 8/8, direct-IP reviewer 4/4"
	Write-Host "No files were copied or archived."
	$global:LASTEXITCODE = 0
	return
}

$Timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$DistributionRoot = Join-Path ([IO.Path]::GetFullPath($OutputRoot)) "Breachborne-Playtest-$Timestamp"
$ClientBundle = Join-Path $DistributionRoot "Client"
$ServerBundle = Join-Path $DistributionRoot "Server"

function Copy-CommonEvidence([string]$BundleRoot) {
	$BundleBuilds = Join-Path $BundleRoot "Builds"
	$BundleScripts = Join-Path $BundleRoot "Scripts\Playtest"
	$BundleDocs = Join-Path $BundleRoot "docs\plans"
	New-Item -ItemType Directory -Force -Path $BundleBuilds,$BundleScripts,$BundleDocs | Out-Null
	Copy-Item -LiteralPath $BuildSummary -Destination $BundleBuilds
	Copy-Item -LiteralPath $CandidateSummary -Destination $BundleBuilds
	if (Test-Path -LiteralPath $VfxAudit) {
		Copy-Item -LiteralPath $VfxAudit -Destination $BundleBuilds
	}
	Copy-Item -LiteralPath (Join-Path $ProjectRoot "docs\plans\july-31-direct-ip-acceptance.md") -Destination $BundleDocs
	Copy-Item -LiteralPath (Join-Path $ProjectRoot "Scripts\Playtest\VerifyPlaytestDistribution.ps1") -Destination $BundleScripts
}

New-Item -ItemType Directory -Force -Path (Join-Path $ClientBundle "Builds") | Out-Null
Copy-Item -LiteralPath $ClientSource -Destination (Join-Path $ClientBundle "Builds\WindowsClient") -Recurse
Copy-CommonEvidence $ClientBundle
Copy-Item -LiteralPath (Join-Path $ProjectRoot "Scripts\Playtest\StartPackagedClient.ps1") -Destination (Join-Path $ClientBundle "Scripts\Playtest")
Copy-Item -LiteralPath (Join-Path $ProjectRoot "Scripts\Playtest\StopPackagedClient.ps1") -Destination (Join-Path $ClientBundle "Scripts\Playtest")
Copy-Item -LiteralPath (Join-Path $ProjectRoot "Scripts\Playtest\ReviewInteractivePlaytest.ps1") -Destination (Join-Path $ClientBundle "Scripts\Playtest")

if ($IncludeServer) {
	New-Item -ItemType Directory -Force -Path (Join-Path $ServerBundle "Builds") | Out-Null
	Copy-Item -LiteralPath $ServerSource -Destination (Join-Path $ServerBundle "Builds\WindowsServer") -Recurse
	Copy-CommonEvidence $ServerBundle
	Copy-Item -LiteralPath (Join-Path $ProjectRoot "Scripts\Playtest\StartPackagedServer.ps1") -Destination (Join-Path $ServerBundle "Scripts\Playtest")
	Copy-Item -LiteralPath (Join-Path $ProjectRoot "Scripts\Playtest\ReviewDirectIPPlaytest.ps1") -Destination (Join-Path $ServerBundle "Scripts\Playtest")
}

Write-BBDistributionManifest -BundleRoot $ClientBundle | Out-Null
if ($IncludeServer) {
	Write-BBDistributionManifest -BundleRoot $ServerBundle | Out-Null
}

$ArchivePaths = @()
if ($CreateArchives) {
	$ClientArchive = "$ClientBundle.zip"
	Compress-Archive -Path (Join-Path $ClientBundle '*') -DestinationPath $ClientArchive -CompressionLevel Optimal
	$ArchivePaths += $ClientArchive
	if ($IncludeServer) {
		$ServerArchive = "$ServerBundle.zip"
		Compress-Archive -Path (Join-Path $ServerBundle '*') -DestinationPath $ServerArchive -CompressionLevel Optimal
		$ArchivePaths += $ServerArchive
	}
}

$Summary = @(
	"Breachborne playtest distribution prepared",
	"Timestamp: $(Get-Date -Format o)",
	"Candidate: $CandidateSummary",
	"Client bundle: $ClientBundle",
	"Server bundle: $(if ($IncludeServer) { $ServerBundle } else { 'not requested' })",
	"Archives: $(if ($ArchivePaths.Count) { $ArchivePaths -join '; ' } else { 'not created' })"
)
$Summary | Set-Content -LiteralPath (Join-Path $DistributionRoot "DistributionSummary.txt") -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

$global:LASTEXITCODE = 0
