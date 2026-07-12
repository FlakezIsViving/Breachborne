param(
	[int]$BasePort = 7851,
	[int]$ConnectionSeconds = 50,
	[string]$ClientBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsClient",
	[string]$ServerBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsServer",
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedHitSmoke"
)

$ErrorActionPreference = "Stop"
$Pairs = @(@(1, 2), @(3, 4), @(5, 6))
$Failures = @()

for ($Index = 0; $Index -lt $Pairs.Count; $Index++) {
	$Pair = $Pairs[$Index]
	$Port = $BasePort + $Index
	Write-Host "Running packaged LMB hit smoke for hunters $($Pair[0]) and $($Pair[1]) on UDP $Port..."
	& (Join-Path $PSScriptRoot "TestPackagedHitSmoke.ps1") `
		-HunterA $Pair[0] `
		-HunterB $Pair[1] `
		-Port $Port `
		-ConnectionSeconds $ConnectionSeconds `
		-ClientBuildRoot $ClientBuildRoot `
		-ServerBuildRoot $ServerBuildRoot `
		-OutputRoot $OutputRoot
	if ($LASTEXITCODE -ne 0) {
		$Failures += "Hunters $($Pair[0])/$($Pair[1]) failed on UDP $Port."
	}
}

if ($Failures.Count -gt 0) {
	Write-Host "Full-roster packaged LMB hit smoke: FAIL"
	$Failures | ForEach-Object { Write-Host "- $_" }
	exit 1
}

Write-Host "Full-roster packaged LMB hit smoke: PASS (hunters 1-6)"
