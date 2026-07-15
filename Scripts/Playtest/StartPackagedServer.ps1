param(
	[string]$BuildRoot = "$PSScriptRoot\..\..\Builds\WindowsServer",
	[string]$Map = "/Game/Maps/TestMap",
	[int]$Port = 7777,
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\DirectIPServer",
	[string[]]$ExtraArgs = @(),
	[switch]$NoSound = $true
)

$ErrorActionPreference = "Stop"

$ResolvedBuildRoot = Resolve-Path -LiteralPath $BuildRoot
$ServerExe = Get-ChildItem -LiteralPath $ResolvedBuildRoot -Recurse -Filter "BreachborneServer.exe" |
	Select-Object -First 1

if (-not $ServerExe) {
	throw "Could not find BreachborneServer.exe under '$ResolvedBuildRoot'. Run Scripts\Packaging\PackageWindowsServer.ps1 first."
}

if (Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue) {
	throw "UDP port $Port is already in use. Stop the existing server or pass another -Port."
}

$Timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$RunDirectory = Join-Path $OutputRoot $Timestamp
New-Item -ItemType Directory -Force -Path $RunDirectory | Out-Null
$ServerLog = Join-Path $RunDirectory "Server.log"

$Args = @(
	$Map,
	"-log",
	"-abslog=`"$ServerLog`"",
	"-port=$Port",
	"-nosteam"
)

if ($NoSound) {
	$Args += "-NoSound"
}
$Args += $ExtraArgs

$StartSummary = @(
	"Breachborne direct-IP packaged server",
	"Started: $(Get-Date -Format o)",
	"Executable: $($ServerExe.FullName)",
	"Executable SHA256: $((Get-FileHash -LiteralPath $ServerExe.FullName -Algorithm SHA256).Hash)",
	"Map: $Map",
	"Port: UDP $Port",
	"Extra args: $(if ($ExtraArgs.Count) { $ExtraArgs -join ' ' } else { 'none' })",
	"Log: $ServerLog"
)
$StartSummary | Set-Content -LiteralPath (Join-Path $RunDirectory "StartSummary.txt") -Encoding UTF8

Write-Host "Starting packaged dedicated server:"
Write-Host "  Exe:  $($ServerExe.FullName)"
Write-Host "  Map:  $Map"
Write-Host "  Port: UDP $Port"
Write-Host "  Evidence: $RunDirectory"

& $ServerExe.FullName @Args
$ServerExitCode = $LASTEXITCODE
@(
	"Stopped: $(Get-Date -Format o)",
	"Exit code: $ServerExitCode"
) | Set-Content -LiteralPath (Join-Path $RunDirectory "ExitSummary.txt") -Encoding UTF8
exit $ServerExitCode
