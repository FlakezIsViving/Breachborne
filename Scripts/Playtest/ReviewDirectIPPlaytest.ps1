param(
	[Parameter(Mandatory = $true)]
	[string]$ServerDirectory,
	[Parameter(Mandatory = $true)]
	[string]$HostClientDirectory,
	[Parameter(Mandatory = $true)]
	[string[]]$RemoteClientDirectories,
	[int]$ExpectedServerJoins = 3
)

$ErrorActionPreference = "Stop"

if ($RemoteClientDirectories.Count -lt 2) {
	throw "Provide the remote client's initial and reconnect evidence directories."
}

$ResolvedServer = (Resolve-Path -LiteralPath $ServerDirectory).ProviderPath
$ResolvedHostClient = (Resolve-Path -LiteralPath $HostClientDirectory).ProviderPath
$ResolvedRemoteClients = @($RemoteClientDirectories | ForEach-Object {
	(Resolve-Path -LiteralPath $_).ProviderPath
})
$ServerLog = Join-Path $ResolvedServer "Server.log"
$HostLog = Join-Path $ResolvedHostClient "Client.log"
$RemoteLogs = @($ResolvedRemoteClients | ForEach-Object { Join-Path $_ "Client.log" })
$RequiredLogs = @($ServerLog, $HostLog) + $RemoteLogs
foreach ($LogPath in $RequiredLogs) {
	if (-not (Test-Path -LiteralPath $LogPath -PathType Leaf)) {
		throw "Required direct-IP log is missing: $LogPath"
	}
}

$RemoteSessions = @($ResolvedRemoteClients | ForEach-Object {
	$SessionPath = Join-Path $_ "Session.json"
	if (-not (Test-Path -LiteralPath $SessionPath -PathType Leaf)) {
		throw "Remote client metadata is missing: $SessionPath"
	}
	Get-Content -LiteralPath $SessionPath -Raw | ConvertFrom-Json
})
$RemoteAddresses = @($RemoteSessions | ForEach-Object { [string]$_.Address } | Sort-Object -Unique)
$RemoteHashes = @($RemoteSessions | ForEach-Object { [string]$_.ExecutableSHA256 } | Sort-Object -Unique)
$NonDirectAddresses = @($RemoteAddresses | Where-Object {
	$_ -match '^(127\.|localhost(?=:|$)|\[?::1\]?(?=:|$))'
})

$CriticalPattern = "Fatal error|Critical error|Ensure condition failed|Assertion failed|OutdatedClient|Network Failure|Travel Failure|Connection TIMED OUT"
$CueOverflowPattern = "GameplayCue.*exceed|GameplayCue.*overflow|Too many GameplayCue|dropped.*GameplayCue|GameplayCue.*no more RPCs are allowed|no more RPCs are allowed.*GameplayCue"
$CriticalFindings = @(Select-String -LiteralPath $RequiredLogs -Pattern $CriticalPattern)
$CueOverflowFindings = @(Select-String -LiteralPath $RequiredLogs -Pattern $CueOverflowPattern)
$ServerJoinCount = @(Select-String -LiteralPath $ServerLog -Pattern "Join succeeded:").Count
$WelcomeCounts = @((@($HostLog) + @($RemoteLogs)) | ForEach-Object {
	@(Select-String -LiteralPath $_ -Pattern "Welcomed by server").Count
})
$HostPlaying = @(Select-String -LiteralPath $HostLog -Pattern "BB_NETFLOW\|CLIENT\|EnterGameplayPhase").Count -gt 0
$RemotePlaying = @(Select-String -LiteralPath $RemoteLogs -Pattern "BB_NETFLOW\|CLIENT\|EnterGameplayPhase").Count -gt 0
$ReconnectRestores = @(Select-String -LiteralPath $ServerLog -Pattern "BB_RECONNECT\|SERVER\|Restored").Count

$Failures = @()
if ($ServerJoinCount -ne $ExpectedServerJoins) {
	$Failures += "Expected exactly $ExpectedServerJoins server joins, observed $ServerJoinCount."
}
if (@($WelcomeCounts | Where-Object { $_ -lt 1 }).Count) {
	$Failures += "Every host/remote client log must contain a server welcome."
}
if (-not $HostPlaying -or -not $RemotePlaying) {
	$Failures += "Host and remote client logs must both reach gameplay."
}
if ($ReconnectRestores -lt 1) {
	$Failures += "Server log has no successful reconnect restoration."
}
if ($RemoteAddresses.Count -ne 1 -or $NonDirectAddresses.Count) {
	$Failures += "Remote initial/reconnect sessions must use one identical non-loopback address."
}
if ($RemoteHashes.Count -ne 1 -or [string]::IsNullOrWhiteSpace($RemoteHashes[0])) {
	$Failures += "Remote initial/reconnect sessions must use one identical recorded executable hash."
}
if ($CriticalFindings.Count) {
	$Failures += "Critical log findings: $($CriticalFindings.Count)."
}
if ($CueOverflowFindings.Count) {
	$Failures += "GameplayCue overflow findings: $($CueOverflowFindings.Count)."
}

$Result = if ($Failures.Count) { "REVIEW" } else { "PASS" }
$SummaryPath = Join-Path $ResolvedServer "DirectIPReview.txt"
$Summary = @(
	"Breachborne direct-IP automated evidence review: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Server evidence: $ResolvedServer",
	"Host client evidence: $ResolvedHostClient",
	"Remote client evidence: $($ResolvedRemoteClients -join '; ')",
	"Remote address: $(if ($RemoteAddresses.Count) { $RemoteAddresses -join '; ' } else { 'missing' })",
	"Server joins: $ServerJoinCount/$ExpectedServerJoins",
	"Client welcomes: $($WelcomeCounts -join ', ')",
	"Host/remote gameplay: $HostPlaying/$RemotePlaying",
	"Reconnect restores: $ReconnectRestores",
	"Remote executable hashes: $($RemoteHashes.Count)",
	"Critical findings: $($CriticalFindings.Count)",
	"GameplayCue overflow findings: $($CueOverflowFindings.Count)",
	"Manual DI-03/DI-04 lobby and visible movement/ability agreement still require tester observation."
)
if ($Failures.Count) {
	$Summary += $Failures | ForEach-Object { "- $_" }
}
$Summary | Set-Content -LiteralPath $SummaryPath -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Result -ne "PASS") {
	exit 1
}
$global:LASTEXITCODE = 0
