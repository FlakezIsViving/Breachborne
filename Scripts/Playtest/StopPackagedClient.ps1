param(
	[string]$SessionDirectory = "",
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\DirectIPClient"
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($SessionDirectory)) {
	$LatestPath = Join-Path $OutputRoot "LatestSession.txt"
	if (-not (Test-Path -LiteralPath $LatestPath)) {
		throw "No direct-IP client session record exists at $LatestPath"
	}
	$SessionDirectory = (Get-Content -LiteralPath $LatestPath -Raw).Trim()
}

$ResolvedSession = (Resolve-Path -LiteralPath $SessionDirectory).ProviderPath
$SessionPath = Join-Path $ResolvedSession "Session.json"
if (-not (Test-Path -LiteralPath $SessionPath)) {
	throw "Direct-IP client metadata not found: $SessionPath"
}

$Session = Get-Content -LiteralPath $SessionPath -Raw | ConvertFrom-Json
$ClientProcess = Get-Process -Id ([int]$Session.PID) -ErrorAction SilentlyContinue
$Result = "already exited"
if ($ClientProcess) {
	$ActualPath = $ClientProcess.Path
	if (-not $ActualPath -or -not [string]::Equals(
		[IO.Path]::GetFullPath($ActualPath),
		[IO.Path]::GetFullPath([string]$Session.Executable),
		[StringComparison]::OrdinalIgnoreCase)) {
		throw "Refusing to stop PID $($Session.PID): executable does not match the recorded client."
	}
	Stop-Process -Id $ClientProcess.Id -Force
	$Result = "stopped PID $($ClientProcess.Id)"
}

$Summary = @(
	"Breachborne direct-IP client stop",
	"Timestamp: $(Get-Date -Format o)",
	"Session: $ResolvedSession",
	"Result: $Result"
)
$Summary | Set-Content -LiteralPath (Join-Path $ResolvedSession "StopSummary.txt") -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }
$global:LASTEXITCODE = 0
