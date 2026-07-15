param(
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\DistributionVerifierSelfTest"
)

$ErrorActionPreference = "Stop"

$ResolvedOutputRoot = [IO.Path]::GetFullPath($OutputRoot)
New-Item -ItemType Directory -Force -Path $ResolvedOutputRoot | Out-Null
$RunDirectory = Join-Path $ResolvedOutputRoot ([guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $RunDirectory | Out-Null

$Verifier = Join-Path $PSScriptRoot "VerifyPlaytestDistribution.ps1"
. (Join-Path $PSScriptRoot "DistributionManifest.ps1")
$PayloadPath = Join-Path $RunDirectory "Payload.txt"
$ManifestPath = Join-Path $RunDirectory "SHA256SUMS.txt"
$PassedScenarios = 0

function Write-TestManifest([string[]]$Lines) {
	$Lines | Set-Content -LiteralPath $ManifestPath -Encoding ASCII
}

function Invoke-ExpectedResult([string]$Scenario, [bool]$ShouldPass) {
	$global:LASTEXITCODE = 0
	& $Verifier -BundleRoot $RunDirectory *> $null
	$Passed = $LASTEXITCODE -eq 0
	if ($Passed -ne $ShouldPass) {
		throw "Distribution verifier scenario '$Scenario' expected pass=$ShouldPass but observed pass=$Passed."
	}
	$script:PassedScenarios++
}

try {
	"distribution verifier self-test" | Set-Content -LiteralPath $PayloadPath -Encoding ASCII
	$Hash = (Get-FileHash -LiteralPath $PayloadPath -Algorithm SHA256).Hash
	$ValidLine = "$Hash  Payload.txt"

	Write-BBDistributionManifest -BundleRoot $RunDirectory | Out-Null
	$GeneratedLines = @(Get-Content -LiteralPath $ManifestPath)
	if ($GeneratedLines.Count -ne 1 -or $GeneratedLines[0] -ne $ValidLine) {
		throw "Generated manifest did not contain the expected normalized payload entry."
	}
	Invoke-ExpectedResult "valid bundle" $true

	"tampered payload" | Set-Content -LiteralPath $PayloadPath -Encoding ASCII
	Invoke-ExpectedResult "hash mismatch" $false
	"distribution verifier self-test" | Set-Content -LiteralPath $PayloadPath -Encoding ASCII

	Write-TestManifest @("$Hash  Missing.txt")
	Invoke-ExpectedResult "missing file" $false

	Write-TestManifest @($ValidLine)
	"unexpected" | Set-Content -LiteralPath (Join-Path $RunDirectory "Unexpected.txt") -Encoding ASCII
	Invoke-ExpectedResult "unexpected file" $false
	Remove-Item -LiteralPath (Join-Path $RunDirectory "Unexpected.txt") -Force

	Write-TestManifest @($ValidLine, $ValidLine)
	Invoke-ExpectedResult "duplicate path" $false

	Write-TestManifest @("$Hash  ..\Outside.txt")
	Invoke-ExpectedResult "path traversal" $false

	Write-TestManifest @("not a manifest line")
	Invoke-ExpectedResult "malformed manifest" $false

	Write-TestManifest @($ValidLine)
	Invoke-ExpectedResult "restored valid bundle" $true
}
finally {
	$ResolvedRunDirectory = (Resolve-Path -LiteralPath $RunDirectory).ProviderPath
	$OutputPrefix = $ResolvedOutputRoot.TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
	if (-not $ResolvedRunDirectory.StartsWith($OutputPrefix, [StringComparison]::OrdinalIgnoreCase)) {
		throw "Refusing to remove self-test directory outside $ResolvedOutputRoot`: $ResolvedRunDirectory"
	}
	Remove-Item -LiteralPath $ResolvedRunDirectory -Recurse -Force
}

Write-Host "Distribution verifier self-test: PASS"
Write-Host "  Scenarios: $PassedScenarios/8"
Write-Host "  Temporary fixture removed: $RunDirectory"
$global:LASTEXITCODE = 0
