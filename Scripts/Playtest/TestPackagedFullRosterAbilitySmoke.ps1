param(
	[int]$BasePort = 7801,
	[int]$ConnectionSeconds = 38,
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedAbilitySmoke",
	[string[]]$ServerExtraArgs = @(),
	[string[]]$ClientExtraArgs = @()
)

$ErrorActionPreference = "Stop"
$Pairs = @(@(1, 2), @(3, 4), @(5, 6))
$Failures = @()

for ($Index = 0; $Index -lt $Pairs.Count; $Index++) {
	$Pair = $Pairs[$Index]
	$Port = $BasePort + $Index
	Write-Host "Running packaged ability smoke for hunters $($Pair[0]) and $($Pair[1]) on UDP $Port..."
	& (Join-Path $PSScriptRoot "TestPackagedAbilitySmoke.ps1") `
		-HunterA $Pair[0] `
		-HunterB $Pair[1] `
		-Port $Port `
		-ConnectionSeconds $ConnectionSeconds `
		-OutputRoot $OutputRoot `
		-ServerExtraArgs $ServerExtraArgs `
		-ClientExtraArgs $ClientExtraArgs
	if ($LASTEXITCODE -ne 0) {
		$Failures += "Hunters $($Pair[0])/$($Pair[1]) failed on UDP $Port."
	}
}

if ($Failures.Count -gt 0) {
	Write-Host "Full-roster packaged ability smoke: FAIL"
	$Failures | ForEach-Object { Write-Host "- $_" }
	exit 1
}

Write-Host "Full-roster packaged ability smoke: PASS (hunters 1-6)"
