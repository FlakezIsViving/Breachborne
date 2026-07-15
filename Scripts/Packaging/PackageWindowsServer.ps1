param(
	[string]$ProjectPath = "$PSScriptRoot\..\..\Breachborne.uproject",
	[string]$ArchiveDirectory = "$PSScriptRoot\..\..\Builds\WindowsServer",
	[string]$Map = "/Game/Maps/TestMap",
	[string]$Config = "Development",
	[string]$RunUAT = ""
)

$ErrorActionPreference = "Stop"

$ResolvedProject = (Resolve-Path -LiteralPath $ProjectPath).ProviderPath
$ResolvedArchiveRoot = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ArchiveDirectory)

if ([string]::IsNullOrWhiteSpace($RunUAT)) {
	$RunUATCandidates = @(
		$env:BREACHBORNE_UE_RUNUAT,
		$(if (-not [string]::IsNullOrWhiteSpace($env:BREACHBORNE_UE_ROOT)) { Join-Path $env:BREACHBORNE_UE_ROOT "Engine\Build\BatchFiles\RunUAT.bat" }),
		"C:\UnrealEngine-5.7.4-release\Engine\Build\BatchFiles\RunUAT.bat"
	) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

	$RunUAT = $RunUATCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
}

if (-not (Test-Path -LiteralPath $RunUAT)) {
	throw "RunUAT.bat not found at '$RunUAT'. Pass -RunUAT or update the script default."
}

New-Item -ItemType Directory -Force -Path $ResolvedArchiveRoot | Out-Null

Write-Host "Packaging Breachborne Windows dedicated server..."
Write-Host "  Project: $ResolvedProject"
Write-Host "  Config:  $Config Server"
Write-Host "  Map:     $Map"
Write-Host "  Output:  $ResolvedArchiveRoot"
Write-Host "  RunUAT:  $RunUAT"

$UATArgs = @(
	"BuildCookRun",
	"-project=$ResolvedProject",
	"-noP4",
	"-server",
	"-serverplatform=Win64",
	"-serverconfig=$Config",
	"-servertarget=BreachborneServer",
	"-noclient",
	"-build",
	"-cook",
	"-map=$Map",
	"-stage",
	"-pak",
	"-archive",
	"-archivedirectory=$ResolvedArchiveRoot"
)

Write-Host "  UAT Args: $($UATArgs -join ' ')"

& $RunUAT @UATArgs

if ($LASTEXITCODE -ne 0) {
	throw "Server packaging failed with exit code $LASTEXITCODE."
}

Write-Host "Server package complete: $ResolvedArchiveRoot"
