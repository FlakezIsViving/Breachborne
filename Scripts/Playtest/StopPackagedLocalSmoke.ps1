param(
	[string]$SessionDirectory = "",
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\InteractivePlaytest",
	[switch]$SkipReview
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
if (-not (Test-Path -LiteralPath $SessionPath)) {
	throw "Session metadata not found: $SessionPath"
}

$Session = Get-Content -LiteralPath $SessionPath -Raw | ConvertFrom-Json
$Stopped = @()
$AlreadyExited = @()

foreach ($Entry in $Session.Processes) {
	$Process = Get-Process -Id ([int]$Entry.PID) -ErrorAction SilentlyContinue
	if (-not $Process) {
		$AlreadyExited += $Entry.Role
		continue
	}

	$ActualPath = $Process.Path
	if (-not $ActualPath -or -not [string]::Equals(
		[IO.Path]::GetFullPath($ActualPath),
		[IO.Path]::GetFullPath([string]$Entry.Executable),
		[System.StringComparison]::OrdinalIgnoreCase)) {
		throw "Refusing to stop PID $($Entry.PID): executable does not match the recorded $($Entry.Role) process."
	}

	Stop-Process -Id $Process.Id -Force
	$Stopped += $Entry.Role
}

$StopSummary = @(
	"Breachborne interactive packaged session stopped.",
	"Timestamp: $(Get-Date -Format o)",
	"Session: $ResolvedSession",
	"Stopped: $(if ($Stopped.Count) { $Stopped -join ', ' } else { 'none' })",
	"Already exited: $(if ($AlreadyExited.Count) { $AlreadyExited -join ', ' } else { 'none' })"
)
$StopSummary | Set-Content -LiteralPath (Join-Path $ResolvedSession "StopSummary.txt") -Encoding UTF8
$StopSummary | ForEach-Object { Write-Host $_ }

if (-not $SkipReview) {
	& (Join-Path $PSScriptRoot "ReviewInteractivePlaytest.ps1") -SessionDirectory $ResolvedSession
	exit $LASTEXITCODE
}
