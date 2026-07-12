param(
	[string]$ProjectPath = "$PSScriptRoot\..\..\Breachborne.uproject",
	[string]$ClientBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsClient",
	[string]$ServerBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsServer",
	[int]$Port = 7789,
	[int]$ClientCount = 2,
	[int]$ServerStartupSeconds = 5,
	[int]$ConnectionSeconds = 20,
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedHandshake",
	[switch]$AbilitySmoke,
	[int[]]$SmokeHunterIDs = @(),
	[string[]]$ServerExtraArgs = @(),
	[string[]]$ClientExtraArgs = @()
)

$ErrorActionPreference = "Stop"

$ClientRoot = (Resolve-Path -LiteralPath $ClientBuildRoot).ProviderPath
$ServerRoot = (Resolve-Path -LiteralPath $ServerBuildRoot).ProviderPath
$ClientExe = Get-ChildItem -LiteralPath $ClientRoot -Recurse -Filter "Breachborne.exe" |
	Where-Object { $_.FullName -match "\\Binaries\\Win64\\" -and $_.FullName -notmatch "Server" } |
	Select-Object -First 1
$ServerExe = Get-ChildItem -LiteralPath $ServerRoot -Recurse -Filter "BreachborneServer.exe" |
	Where-Object { $_.FullName -match "\\Binaries\\Win64\\" } |
	Select-Object -First 1

if (-not $ClientExe -or -not $ServerExe) {
	throw "Packaged inner executables were not found. Run Scripts\Packaging\PackagePlaytestWindows.ps1 first."
}

$HandshakeScript = Join-Path $PSScriptRoot "..\Networking\TestHeadlessDedicatedHandshake.ps1"
& $HandshakeScript `
	-ProjectPath $ProjectPath `
	-Port $Port `
	-ClientCount $ClientCount `
	-ServerStartupSeconds $ServerStartupSeconds `
	-ConnectionSeconds $ConnectionSeconds `
	-OutputRoot $OutputRoot `
	-ServerExePath $ServerExe.FullName `
	-ClientExePath $ClientExe.FullName `
	-Packaged `
	-AbilitySmoke:$AbilitySmoke `
	-SmokeHunterIDs $SmokeHunterIDs `
	-ServerExtraArgs $ServerExtraArgs `
	-ClientExtraArgs $ClientExtraArgs

exit $LASTEXITCODE
