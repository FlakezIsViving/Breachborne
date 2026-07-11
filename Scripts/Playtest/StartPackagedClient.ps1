param(
	[string]$BuildRoot = "$PSScriptRoot\..\..\Builds\WindowsClient",
	[string]$Address = "127.0.0.1:7777",
	[int]$ResX = 1280,
	[int]$ResY = 720,
	[int]$WinX = 80,
	[int]$WinY = 80
)

$ErrorActionPreference = "Stop"

$ResolvedBuildRoot = Resolve-Path -LiteralPath $BuildRoot
$ClientExe = Get-ChildItem -LiteralPath $ResolvedBuildRoot -Recurse -Filter "Breachborne.exe" |
	Where-Object { $_.FullName -notmatch "Server" } |
	Select-Object -First 1

if (-not $ClientExe) {
	throw "Could not find Breachborne.exe under '$ResolvedBuildRoot'. Run Scripts\Packaging\PackageWindowsClient.ps1 first."
}

$Args = @(
	$Address,
	"-game",
	"-log",
	"-windowed",
	"-ResX=$ResX",
	"-ResY=$ResY",
	"-WinX=$WinX",
	"-WinY=$WinY",
	"-nosteam"
)

Write-Host "Starting packaged client:"
Write-Host "  Exe:     $($ClientExe.FullName)"
Write-Host "  Address: $Address"

Start-Process -FilePath $ClientExe.FullName -ArgumentList $Args
