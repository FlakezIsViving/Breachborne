param(
	[string]$ProjectPath = "$PSScriptRoot\..\..\Breachborne.uproject",
	[string]$ArchiveDirectory = "$PSScriptRoot\..\..\Builds\WindowsClient",
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

Write-Host "Packaging Breachborne Windows client..."
Write-Host "  Project: $ResolvedProject"
Write-Host "  Config:  $Config"
Write-Host "  Map:     $Map"
Write-Host "  Output:  $ResolvedArchiveRoot"
Write-Host "  RunUAT:  $RunUAT"

$UATArgs = @(
	"BuildCookRun",
	"-project=$ResolvedProject",
	"-noP4",
	"-platform=Win64",
	"-clientconfig=$Config",
	"-target=Breachborne",
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
	throw "Client packaging failed with exit code $LASTEXITCODE."
}

Write-Host "Client package complete: $ResolvedArchiveRoot"
