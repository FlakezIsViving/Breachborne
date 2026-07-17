param(
	[string]$ProjectPath = "$PSScriptRoot\..\..\Breachborne.uproject",
	[string]$EngineRoot = "C:\UnrealEngine-5.7.4-release",
	[switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

function Invoke-BBProcess {
	param(
		[Parameter(Mandatory = $true)]
		[string]$FilePath,
		[Parameter(Mandatory = $true)]
		[string[]]$Arguments,
		[Parameter(Mandatory = $true)]
		[string]$Description
	)

	Write-Host ""
	Write-Host $Description
	Write-Host "  $FilePath $($Arguments -join ' ')"
	& $FilePath @Arguments
	if ($LASTEXITCODE -ne 0) {
		throw "$Description failed with exit code $LASTEXITCODE."
	}
}

function Assert-BBLatestLogMarker {
	param(
		[Parameter(Mandatory = $true)]
		[string]$LogsRoot,
		[Parameter(Mandatory = $true)]
		[string]$Pattern,
		[Parameter(Mandatory = $true)]
		[string]$Description
	)

	$LatestLog = Get-ChildItem -LiteralPath $LogsRoot -Filter "*.log" -File |
		Sort-Object LastWriteTime -Descending |
		Select-Object -First 1
	if (-not $LatestLog) {
		throw "$Description produced no project log under '$LogsRoot'."
	}
	if (-not (Select-String -LiteralPath $LatestLog.FullName -Pattern $Pattern -Quiet)) {
		throw "$Description did not emit '$Pattern' in '$($LatestLog.FullName)'."
	}
	Write-Host "  Verified: $Pattern"
}

$RunningEditor = Get-Process -Name "UnrealEditor*" -ErrorAction SilentlyContinue
if ($RunningEditor) {
	$ProcessList = ($RunningEditor | ForEach-Object { "$($_.Id)" }) -join ", "
	throw "Close Unreal Editor before repairing content. Running PID(s): $ProcessList"
}

$ResolvedProject = (Resolve-Path -LiteralPath $ProjectPath).ProviderPath
$ResolvedEngine = (Resolve-Path -LiteralPath $EngineRoot).ProviderPath
$ProjectRoot = Split-Path -Parent $ResolvedProject
$BuildTool = Join-Path $ResolvedEngine "Engine\Build\BatchFiles\Build.bat"
$EditorCmd = Join-Path $ResolvedEngine "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$PythonRepair = Join-Path $ProjectRoot "Scripts\Debug\RepairLudus10ContentCompatibility.py"
$LogsRoot = Join-Path $ProjectRoot "Saved\Logs"
$HudsonContentFolder = Join-Path $ProjectRoot "Content\Hunters\Hudson"
$HudsonTposeSourceFile = Join-Path $HudsonContentFolder "Hudson_TPOSE.uasset"
$HudsonTposeDestinationFile = Join-Path $HudsonContentFolder "Meshes\Hudson_TPOSE.uasset"

foreach ($RequiredPath in @($BuildTool, $EditorCmd, $PythonRepair)) {
	if (-not (Test-Path -LiteralPath $RequiredPath -PathType Leaf)) {
		throw "Required repair input not found: $RequiredPath"
	}
}

if (-not $SkipBuild) {
	Invoke-BBProcess -FilePath $BuildTool -Description "Building BreachborneEditor" -Arguments @(
		"BreachborneEditor",
		"Win64",
		"Development",
		"-Project=$ResolvedProject",
		"-WaitMutex",
		"-NoHotReloadFromIDE"
	)
}

$CommonArguments = @(
	$ResolvedProject,
	"-DisablePlugins=LudusAI",
	"-unattended",
	"-nop4",
	"-nosplash",
	"-nullrhi",
	"-nosound",
	"-log"
)

for ($Pass = 1; $Pass -le 2; $Pass++) {
	Invoke-BBProcess -FilePath $EditorCmd -Description "Running LUDUS package-path repair (pass $Pass/2)" -Arguments @(
		$CommonArguments + @("-ExecutePythonScript=$PythonRepair")
	)
	Assert-BBLatestLogMarker -LogsRoot $LogsRoot `
		-Pattern "BB_LUDUS_REPAIR\|COMPLETE" `
		-Description "LUDUS package-path repair"

	if (Test-Path -LiteralPath $HudsonTposeSourceFile) {
		Assert-BBLatestLogMarker -LogsRoot $LogsRoot `
			-Pattern "BB_LUDUS_REPAIR\|HUDSON_TPOSE_REDIRECTOR\|REMOVED" `
			-Description "Hudson T-pose redirector validation"
		$ResolvedSourceFile = (Resolve-Path -LiteralPath $HudsonTposeSourceFile).ProviderPath
		$ExpectedSourceFile = [System.IO.Path]::GetFullPath($HudsonTposeSourceFile)
		if ($ResolvedSourceFile -ne $ExpectedSourceFile -or
			-not $ResolvedSourceFile.StartsWith($HudsonContentFolder, [System.StringComparison]::OrdinalIgnoreCase)) {
			throw "Refusing to delete unexpected redirector path: $ResolvedSourceFile"
		}
		Remove-Item -LiteralPath $ResolvedSourceFile -Force
		Write-Host "  Removed validated Hudson T-pose redirector package"
	}
	if (-not (Test-Path -LiteralPath $HudsonTposeDestinationFile -PathType Leaf)) {
		throw "Hudson T-pose destination is missing after pass $Pass`: $HudsonTposeDestinationFile"
	}
	Write-Host "  Verified: Hudson T-pose source redirector absent; destination present"

	Invoke-BBProcess -FilePath $EditorCmd -Description "Running Hudson skeleton repair (pass $Pass/2)" -Arguments @(
		$CommonArguments + @("-run=BBRepairLudusContent")
	)
	$ExpectedSocketMarker = if ($Pass -eq 1) {
		"BB_LUDUS_REPAIR\|HUDSON_NULL_SOCKETS\|REMOVED=[0-9]+"
	}
	else {
		"BB_LUDUS_REPAIR\|HUDSON_NULL_SOCKETS\|REMOVED=0"
	}
	Assert-BBLatestLogMarker -LogsRoot $LogsRoot `
		-Pattern $ExpectedSocketMarker `
		-Description "Hudson skeleton repair"
}

Write-Host ""
Write-Host "LUDUS 1.0 content compatibility repair: PASS"
Write-Host "  Project: $ResolvedProject"
Write-Host "  Engine:  $ResolvedEngine"
Write-Host "  Package paths: repaired and idempotent"
Write-Host "  Hudson skeleton: null sockets removed and idempotent"
