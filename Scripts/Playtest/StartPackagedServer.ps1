param(
	[string]$BuildRoot = "$PSScriptRoot\..\..\Builds\WindowsServer",
	[string]$Map = "/Game/Maps/TestMap",
	[int]$Port = 7777,
	[switch]$NoSound = $true
)

$ErrorActionPreference = "Stop"

$ResolvedBuildRoot = Resolve-Path -LiteralPath $BuildRoot
$ServerExe = Get-ChildItem -LiteralPath $ResolvedBuildRoot -Recurse -Filter "BreachborneServer.exe" |
	Select-Object -First 1

if (-not $ServerExe) {
	throw "Could not find BreachborneServer.exe under '$ResolvedBuildRoot'. Run Scripts\Packaging\PackageWindowsServer.ps1 first."
}

$Args = @(
	$Map,
	"-log",
	"-port=$Port",
	"-nosteam"
)

if ($NoSound) {
	$Args += "-NoSound"
}

Write-Host "Starting packaged dedicated server:"
Write-Host "  Exe:  $($ServerExe.FullName)"
Write-Host "  Map:  $Map"
Write-Host "  Port: UDP $Port"
Write-Host "  Logs: $((Split-Path -Parent $ServerExe.FullName))\..\..\Saved\Logs"

& $ServerExe.FullName @Args
