param(
	[string]$ProjectRoot = (Resolve-Path "$PSScriptRoot\..\.."),
	[string]$OutputPath = "$PSScriptRoot\..\..\Builds\VfxFoundationAudit.txt"
)

$ErrorActionPreference = "Stop"

$Root = (Resolve-Path -LiteralPath $ProjectRoot).ProviderPath
$ContentRoot = Join-Path $Root "Content"
$TemplateRoot = Join-Path $ContentRoot "GameplayCues\Templates"
$GameConfigPath = Join-Path $Root "Config\DefaultGame.ini"
$ScalabilityPath = Join-Path $Root "Config\DefaultScalability.ini"
$VisualContractPath = Join-Path $Root "Source\Breachborne\Abilities\BBAbilityVisualSet.h"

$Templates = @(
	"NS_BB_ProjectileBody",
	"NS_BB_ProjectileTrail",
	"NS_BB_BeamTether",
	"NS_BB_GroundTelegraph",
	"NS_BB_PersistentGroundZone",
	"NS_BB_ConeWedge",
	"NS_BB_ImpactBurst",
	"NS_BB_CharacterAura"
)

$TemplateResults = foreach ($Template in $Templates) {
	$Path = Join-Path $TemplateRoot "$Template.uasset"
	[pscustomobject]@{ Name = $Template; Path = $Path; Present = Test-Path -LiteralPath $Path }
}

$GameConfig = Get-Content -LiteralPath $GameConfigPath -Raw
$Scalability = Get-Content -LiteralPath $ScalabilityPath -Raw
$VisualContract = Get-Content -LiteralPath $VisualContractPath -Raw

$CueRoots = @("/Game/GameplayCues", "/Game/Hunters", "/Game/Powers")
$CueRootResults = foreach ($CueRoot in $CueRoots) {
	[pscustomobject]@{ Name = $CueRoot; Present = $GameConfig.Contains($CueRoot) }
}

$FallbackFiles = @(
	"Source\Breachborne\Combat\BBPrimitiveBeamActor.cpp",
	"Source\Breachborne\Combat\BBPrimitiveBurstActor.cpp",
	"Source\Breachborne\Combat\BBPrimitiveWedgeActor.cpp"
)
$FallbackResults = foreach ($RelativePath in $FallbackFiles) {
	[pscustomobject]@{ Name = $RelativePath; Present = Test-Path -LiteralPath (Join-Path $Root $RelativePath) }
}

$TemplateEnumValues = @(
	"ProjectileBody", "ProjectileTrail", "BeamTether", "GroundTelegraph",
	"PersistentGroundZone", "ConeWedge", "ImpactBurst", "CharacterAura"
)
$EnumContractPassed = @($TemplateEnumValues | Where-Object { -not $VisualContract.Contains($_) }).Count -eq 0
$LowQualityConfigured = $Scalability.Contains("fx.Niagara.QualityLevel=0")
$MediumQualityConfigured = $Scalability.Contains("fx.Niagara.QualityLevel=1")
$CueRootsPassed = @($CueRootResults | Where-Object { -not $_.Present }).Count -eq 0
$FallbacksPassed = @($FallbackResults | Where-Object { -not $_.Present }).Count -eq 0
$AuthoredCount = @($TemplateResults | Where-Object Present).Count
$FoundationReady = $CueRootsPassed -and $FallbacksPassed -and $EnumContractPassed -and $LowQualityConfigured -and $MediumQualityConfigured
$Status = if (-not $FoundationReady) { "RED" } elseif ($AuthoredCount -eq $Templates.Count) { "GREEN" } else { "FALLBACK_READY_AUTHORING_INCOMPLETE" }

$Lines = @(
	"Breachborne VFX foundation audit: $Status",
	"Timestamp: $(Get-Date -Format o)",
	"Project: $Root",
	"GameplayCue roots: $(if ($CueRootsPassed) { 'PASS' } else { 'FAIL' })",
	"Template enum contract: $(if ($EnumContractPassed) { 'PASS' } else { 'FAIL' })",
	"Primitive fallback files: $(if ($FallbacksPassed) { 'PASS' } else { 'FAIL' })",
	"Low/Medium Niagara scalability: $(if ($LowQualityConfigured -and $MediumQualityConfigured) { 'PASS' } else { 'FAIL' })",
	"Authored Niagara masters: $AuthoredCount/$($Templates.Count)",
	"",
	"Master assets:"
)
$Lines += $TemplateResults | ForEach-Object { "- $($_.Name): $(if ($_.Present) { 'PRESENT' } else { 'MISSING' }) [$($_.Path)]" }
$Lines += ""
$Lines += "Cue roots:"
$Lines += $CueRootResults | ForEach-Object { "- $($_.Name): $(if ($_.Present) { 'PASS' } else { 'MISSING' })" }
$Lines += ""
$Lines += "Primitive fallbacks:"
$Lines += $FallbackResults | ForEach-Object { "- $($_.Name): $(if ($_.Present) { 'PASS' } else { 'MISSING' })" }
$Lines += ""
$Lines += "A fallback-ready result supports the primitive July 31 candidate. It does not complete the authored-Niagara gate or manual readability acceptance."

$ResolvedOutput = [IO.Path]::GetFullPath($OutputPath)
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $ResolvedOutput) | Out-Null
$Lines | Set-Content -LiteralPath $ResolvedOutput -Encoding UTF8
$Lines | ForEach-Object { Write-Host $_ }

if (-not $FoundationReady) {
	exit 1
}

$global:LASTEXITCODE = 0
