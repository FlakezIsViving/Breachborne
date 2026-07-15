param(
	[int]$BasePort = 7861,
	[int]$ConnectionSeconds = 55,
	[string]$ClientBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsClient",
	[string]$ServerBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsServer",
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedOutcomeSmoke",
	[string[]]$ServerExtraArgs = @(),
	[string[]]$ClientExtraArgs = @()
)

$ErrorActionPreference = "Stop"
$Failures = @()

for ($HunterID = 1; $HunterID -le 6; $HunterID++) {
	$TargetHunterID = if ($HunterID -eq 1) { 2 } else { 1 }
	$Port = $BasePort + $HunterID - 1
	Write-Host "Running packaged non-LMB outcome smoke for hunter $HunterID on UDP $Port..."
	& (Join-Path $PSScriptRoot "TestPackagedOutcomeSmoke.ps1") `
		-HunterID $HunterID `
		-TargetHunterID $TargetHunterID `
		-Port $Port `
		-ConnectionSeconds $ConnectionSeconds `
		-ClientBuildRoot $ClientBuildRoot `
		-ServerBuildRoot $ServerBuildRoot `
		-OutputRoot $OutputRoot `
		-ServerExtraArgs $ServerExtraArgs `
		-ClientExtraArgs $ClientExtraArgs
	if ($LASTEXITCODE -ne 0) {
		$Failures += "Hunter $HunterID failed on UDP $Port."
	}
}

if ($Failures.Count -gt 0) {
	Write-Host "Full-roster packaged non-LMB outcome smoke: FAIL"
	$Failures | ForEach-Object { Write-Host "- $_" }
	exit 1
}

Write-Host "Full-roster packaged non-LMB outcome smoke: PASS (24/24 contracts)"
