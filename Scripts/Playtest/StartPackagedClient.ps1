param(
	[string]$BuildRoot = "$PSScriptRoot\..\..\Builds\WindowsClient",
	[string]$Address = "127.0.0.1:7777",
	[int]$ResX = 1280,
	[int]$ResY = 720,
	[int]$WinX = 80,
	[int]$WinY = 80,
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\DirectIPClient",
	[string[]]$ExtraArgs = @()
)

$ErrorActionPreference = "Stop"

$ResolvedBuildRoot = Resolve-Path -LiteralPath $BuildRoot
$ClientExe = Get-ChildItem -LiteralPath $ResolvedBuildRoot -Recurse -Filter "Breachborne.exe" |
	Where-Object { $_.FullName -notmatch "Server" } |
	Select-Object -First 1

if (-not $ClientExe) {
	throw "Could not find Breachborne.exe under '$ResolvedBuildRoot'. Run Scripts\Packaging\PackageWindowsClient.ps1 first."
}

$Timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$RunDirectory = Join-Path $OutputRoot $Timestamp
New-Item -ItemType Directory -Force -Path $RunDirectory | Out-Null
$ClientLog = Join-Path $RunDirectory "Client.log"

$Args = @(
	$Address,
	"-game",
	"-log",
	"-abslog=`"$ClientLog`"",
	"-windowed",
	"-ResX=$ResX",
	"-ResY=$ResY",
	"-WinX=$WinX",
	"-WinY=$WinY",
	"-nosteam"
)
$Args += $ExtraArgs

Write-Host "Starting packaged client:"
Write-Host "  Exe:     $($ClientExe.FullName)"
Write-Host "  Address: $Address"
Write-Host "  Evidence: $RunDirectory"

$Client = Start-Process -FilePath $ClientExe.FullName -ArgumentList $Args -PassThru
$Session = [ordered]@{
	StartedAt = (Get-Date -Format o)
	Address = $Address
	PID = $Client.Id
	Executable = $ClientExe.FullName
	ExecutableSHA256 = (Get-FileHash -LiteralPath $ClientExe.FullName -Algorithm SHA256).Hash
	ExtraArgs = $ExtraArgs
	Log = $ClientLog
}
$Session | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $RunDirectory "Session.json") -Encoding UTF8
$RunDirectory | Set-Content -LiteralPath (Join-Path $OutputRoot "LatestSession.txt") -Encoding UTF8
