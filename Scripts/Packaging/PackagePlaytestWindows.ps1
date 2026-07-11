param(
	[string]$ProjectPath = "$PSScriptRoot\..\..\Breachborne.uproject",
	[string]$BuildsRoot = "$PSScriptRoot\..\..\Builds",
	[string]$Map = "/Game/Maps/TestMap",
	[string]$Config = "Development",
	[string]$RunUAT = "",
	[switch]$SkipServer
)

$ErrorActionPreference = "Stop"

$ResolvedBuildsRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($BuildsRoot)
$ClientOutput = Join-Path $ResolvedBuildsRoot "WindowsClient"
$ServerOutput = Join-Path $ResolvedBuildsRoot "WindowsServer"
$SummaryPath = Join-Path $ResolvedBuildsRoot "PlaytestBuildSummary.txt"

& "$PSScriptRoot\PackageWindowsClient.ps1" `
	-ProjectPath $ProjectPath `
	-ArchiveDirectory $ClientOutput `
	-Map $Map `
	-Config $Config `
	-RunUAT $RunUAT

if (-not $SkipServer) {
	& "$PSScriptRoot\PackageWindowsServer.ps1" `
		-ProjectPath $ProjectPath `
		-ArchiveDirectory $ServerOutput `
		-Map $Map `
		-Config $Config `
		-RunUAT $RunUAT
}
else {
	Write-Host "Skipping Windows dedicated server package because -SkipServer was provided."
}

New-Item -ItemType Directory -Force -Path $ResolvedBuildsRoot | Out-Null

$Summary = @(
	"Breachborne Windows playtest build",
	"Timestamp: $(Get-Date -Format o)",
	"Config: $Config",
	"Map: $Map",
	"Client: $ClientOutput",
	"Server: $(if ($SkipServer) { 'SKIPPED' } else { $ServerOutput })",
	"Direct IP port: UDP 7777"
)

$Summary | Set-Content -LiteralPath $SummaryPath -Encoding UTF8

Write-Host ""
Write-Host ($Summary -join [Environment]::NewLine)
Write-Host ""
Write-Host "Summary written to: $SummaryPath"
