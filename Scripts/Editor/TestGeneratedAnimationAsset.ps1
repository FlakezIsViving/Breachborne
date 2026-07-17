param(
	[string]$AssetPath = "/Game/Animations/A_Ghost_Shooting",
	[string]$ExpectedSkeletonPath = "/Game/Characters/SK_GhostSpecOps_Skeleton",
	[ValidateSet("AnimSequence", "AnimMontage")]
	[string]$ExpectedClass = "AnimSequence",
	[string]$RequiredSlotName = "DefaultSlot",
	[switch]$RequireLoop,
	[string]$ProjectPath = "$PSScriptRoot\..\..\Breachborne.uproject",
	[string]$EngineRoot = "C:\UnrealEngine-5.7.4-release"
)

$ErrorActionPreference = "Stop"

$RunningEditor = Get-Process -Name "UnrealEditor*" -ErrorAction SilentlyContinue
if ($RunningEditor) {
	$ProcessList = ($RunningEditor | ForEach-Object { $_.Id }) -join ", "
	throw "Close Unreal Editor before auditing generated animation assets. Running PID(s): $ProcessList"
}

$ResolvedProject = (Resolve-Path -LiteralPath $ProjectPath).ProviderPath
$ResolvedEngine = (Resolve-Path -LiteralPath $EngineRoot).ProviderPath
$EditorCmd = Join-Path $ResolvedEngine "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$AuditScript = Join-Path $PSScriptRoot "AuditGeneratedAnimation.py"
foreach ($RequiredPath in @($EditorCmd, $AuditScript)) {
	if (-not (Test-Path -LiteralPath $RequiredPath -PathType Leaf)) {
		throw "Required animation-audit input not found: $RequiredPath"
	}
}

$ProjectDescriptor = Get-Content -LiteralPath $ResolvedProject -Raw | ConvertFrom-Json
$DisabledPlugins = @(
	$ProjectDescriptor.Plugins |
		Where-Object { $_.Enabled -eq $false } |
		ForEach-Object { [string]$_.Name }
)
if ($DisabledPlugins -notcontains "LudusAI") {
	$DisabledPlugins += "LudusAI"
}

$EvidenceRoot = Join-Path (Split-Path -Parent $ResolvedProject) "Saved\Logs\AnimationAudit"
New-Item -ItemType Directory -Force -Path $EvidenceRoot | Out-Null
$EvidenceDirectory = Join-Path $EvidenceRoot (Get-Date -Format "yyyyMMdd-HHmmss")
New-Item -ItemType Directory -Force -Path $EvidenceDirectory | Out-Null
$LogPath = Join-Path $EvidenceDirectory "AnimationAudit.log"

$PreviousEnvironment = @{
	BB_ANIMATION_ASSET = $env:BB_ANIMATION_ASSET
	BB_ANIMATION_SKELETON = $env:BB_ANIMATION_SKELETON
	BB_ANIMATION_CLASS = $env:BB_ANIMATION_CLASS
	BB_ANIMATION_SLOT = $env:BB_ANIMATION_SLOT
	BB_ANIMATION_REQUIRE_LOOP = $env:BB_ANIMATION_REQUIRE_LOOP
}

try {
	$env:BB_ANIMATION_ASSET = $AssetPath
	$env:BB_ANIMATION_SKELETON = $ExpectedSkeletonPath
	$env:BB_ANIMATION_CLASS = $ExpectedClass
	$env:BB_ANIMATION_SLOT = if ($ExpectedClass -eq "AnimMontage") { $RequiredSlotName } else { "" }
	$env:BB_ANIMATION_REQUIRE_LOOP = if ($RequireLoop) { "1" } else { "0" }

	$Arguments = @(
		$ResolvedProject,
		"-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities",
		"-DisablePlugins=$($DisabledPlugins -join ',')",
		"-unattended",
		"-nop4",
		"-nosplash",
		"-nullrhi",
		"-nosound",
		"-abslog=$LogPath",
		"-ExecutePythonScript=$AuditScript"
	)
	& $EditorCmd @Arguments
	if ($LASTEXITCODE -ne 0) {
		throw "Generated-animation audit exited with code $LASTEXITCODE. See $LogPath"
	}
}
finally {
	foreach ($Entry in $PreviousEnvironment.GetEnumerator()) {
		if ($null -eq $Entry.Value) {
			Remove-Item -Path "Env:$($Entry.Key)" -ErrorAction SilentlyContinue
		}
		else {
			Set-Item -Path "Env:$($Entry.Key)" -Value $Entry.Value
		}
	}
}

if (-not (Select-String -LiteralPath $LogPath -Pattern "BB_ANIMATION_AUDIT\|PASS" -Quiet)) {
	throw "Generated-animation audit emitted no PASS marker. See $LogPath"
}

Write-Host "Generated animation asset audit: PASS"
Write-Host "  Asset: $AssetPath"
Write-Host "  Class: $ExpectedClass"
Write-Host "  Skeleton: $ExpectedSkeletonPath"
Write-Host "  Evidence: $EvidenceDirectory"
