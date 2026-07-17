param(
	[string]$ProjectPath = "$PSScriptRoot\..\..\Breachborne.uproject",
	[string]$EngineRoot = "C:\UnrealEngine-5.7.4-release",
	[string]$GhostSkeletonPath = "/Game/Characters/SK_GhostSpecOps_Skeleton",
	[string[]]$GhostAnimationAssets = @(),
	[switch]$DryRun
)

$ErrorActionPreference = "Stop"

$ResolvedProject = (Resolve-Path -LiteralPath $ProjectPath).ProviderPath
$ResolvedEngine = (Resolve-Path -LiteralPath $EngineRoot).ProviderPath
$ProjectRoot = Split-Path -Parent $ResolvedProject
$BuildTool = Join-Path $ResolvedEngine "Engine\Build\BatchFiles\Build.bat"
$AutomationRunner = Join-Path $ProjectRoot "Scripts\Testing\RunEditorAutomation.ps1"
$AnimationAudit = Join-Path $ProjectRoot "Scripts\Editor\TestGeneratedAnimationAsset.ps1"
$RepositoryPolicy = Join-Path $ProjectRoot "Scripts\SourceControl\TestRepositoryAssetPolicy.ps1"
$ManualLauncherContracts = Join-Path $ProjectRoot "Scripts\Playtest\TestManualAcceptanceLaunchers.ps1"
foreach ($RequiredPath in @(
	$BuildTool,
	$AutomationRunner,
	$AnimationAudit,
	$RepositoryPolicy,
	$ManualLauncherContracts
)) {
	if (-not (Test-Path -LiteralPath $RequiredPath -PathType Leaf)) {
		throw "Required post-authoring input not found: $RequiredPath"
	}
}

$AnimationDirectory = Join-Path $ProjectRoot "Content\Animations"
$DiscoveredGhostAnimations = @(
	Get-ChildItem -LiteralPath $AnimationDirectory -File -Filter "A_Ghost_*.uasset" -ErrorAction SilentlyContinue |
		ForEach-Object { "/Game/Animations/$($_.BaseName)" }
)
$GhostAnimationAssets = @(
	$GhostAnimationAssets + $DiscoveredGhostAnimations |
		Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
		Sort-Object -Unique
)
if ($GhostAnimationAssets.Count -eq 0) {
	throw "No Ghost animation sequences were supplied or discovered under $AnimationDirectory."
}

$Steps = @(
	"Run manual acceptance launcher contracts (7 modes / 6 ledgers)",
	"Build BreachborneEditor Win64 Development",
	"Run Breachborne.PureLogic (47 tests)",
	"Run Breachborne.GameSystems (55 tests)"
)
$Steps += $GhostAnimationAssets | ForEach-Object { "Audit generated animation $_" }
$Steps += "Run repository asset policy"

Write-Host "Breachborne post-authoring validation"
$Steps | ForEach-Object { Write-Host "- $_" }
if ($DryRun) {
	return
}

$RunningEditor = Get-Process -Name "UnrealEditor*" -ErrorAction SilentlyContinue
if ($RunningEditor) {
	$ProcessList = ($RunningEditor | ForEach-Object { $_.Id }) -join ", "
	throw "Close Unreal Editor before post-authoring validation. Running PID(s): $ProcessList"
}

$EvidenceRoot = Join-Path $ProjectRoot "Saved\Logs\PostAuthoringValidation"
$EvidenceDirectory = Join-Path $EvidenceRoot (Get-Date -Format "yyyyMMdd-HHmmss")
New-Item -ItemType Directory -Force -Path $EvidenceDirectory | Out-Null
$SummaryPath = Join-Path $EvidenceDirectory "Summary.txt"
$CompletedSteps = [Collections.Generic.List[string]]::new()

try {
	& $ManualLauncherContracts
	if ($LASTEXITCODE -ne 0) {
		throw "Manual acceptance launcher contracts failed."
	}
	$CompletedSteps.Add("Manual launcher contracts 7/7 and ledgers 6/6 PASS")

	& $BuildTool BreachborneEditor Win64 Development "-Project=$ResolvedProject" -WaitMutex -NoHotReloadFromIDE
	if ($LASTEXITCODE -ne 0) {
		throw "BreachborneEditor build failed with exit code $LASTEXITCODE."
	}
	$CompletedSteps.Add("Editor build PASS")

	& $AutomationRunner -TestFilter "Breachborne.PureLogic" -ExpectedTestCount 47 `
		-ProjectPath $ResolvedProject -EngineRoot $ResolvedEngine
	if ($LASTEXITCODE -ne 0) {
		throw "PureLogic automation failed."
	}
	$CompletedSteps.Add("PureLogic 47/47 PASS")

	& $AutomationRunner -TestFilter "Breachborne.GameSystems" -ExpectedTestCount 55 `
		-ProjectPath $ResolvedProject -EngineRoot $ResolvedEngine
	if ($LASTEXITCODE -ne 0) {
		throw "GameSystems automation failed."
	}
	$CompletedSteps.Add("GameSystems 55/55 PASS")

	foreach ($AssetPath in $GhostAnimationAssets) {
		& $AnimationAudit -AssetPath $AssetPath -ExpectedSkeletonPath $GhostSkeletonPath `
			-ExpectedClass AnimSequence -ProjectPath $ResolvedProject -EngineRoot $ResolvedEngine
		if ($LASTEXITCODE -ne 0) {
			throw "Animation audit failed for $AssetPath."
		}
		$CompletedSteps.Add("Animation audit PASS: $AssetPath")
	}

	& $RepositoryPolicy
	if ($LASTEXITCODE -ne 0) {
		throw "Repository asset policy failed."
	}
	$CompletedSteps.Add("Repository asset policy PASS")

	$Summary = @(
		"Breachborne post-authoring validation: PASS",
		"Timestamp: $(Get-Date -Format o)",
		"Project: $ResolvedProject",
		"Engine: $ResolvedEngine"
	) + $CompletedSteps
	$Summary | Set-Content -LiteralPath $SummaryPath -Encoding UTF8
	$Summary | ForEach-Object { Write-Host $_ }
}
catch {
	$Summary = @(
		"Breachborne post-authoring validation: FAIL",
		"Timestamp: $(Get-Date -Format o)",
		"Failure: $($_.Exception.Message)",
		"Completed steps:"
	) + $CompletedSteps
	$Summary | Set-Content -LiteralPath $SummaryPath -Encoding UTF8
	$Summary | ForEach-Object { Write-Host $_ }
	throw
}
