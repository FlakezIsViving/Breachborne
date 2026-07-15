param(
	[int]$Port = 7871,
	[int]$ConnectionSeconds = 20,
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedFourClientHandshake"
)

$ErrorActionPreference = "Stop"

$ExistingRunDirectories = @{}
if (Test-Path -LiteralPath $OutputRoot) {
	Get-ChildItem -LiteralPath $OutputRoot -Directory | ForEach-Object {
		$ExistingRunDirectories[$_.FullName] = $true
	}
}

& (Join-Path $PSScriptRoot "TestPackagedHeadlessHandshake.ps1") `
	-Port $Port `
	-ClientCount 4 `
	-ServerStartupSeconds 4 `
	-ConnectionSeconds $ConnectionSeconds `
	-OutputRoot $OutputRoot
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$RunDirectory = Get-ChildItem -LiteralPath $OutputRoot -Directory |
	Where-Object { -not $ExistingRunDirectories.ContainsKey($_.FullName) } |
	Sort-Object CreationTime -Descending |
	Select-Object -First 1
if (-not $RunDirectory) {
	throw "A new four-client handshake evidence directory was not created."
}

& (Join-Path $PSScriptRoot "ReviewInteractivePlaytest.ps1") -SessionDirectory $RunDirectory.FullName
if ($LASTEXITCODE -ne 0) {
	exit $LASTEXITCODE
}

$HandshakeText = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "Summary.txt") -Raw
$ReviewText = Get-Content -LiteralPath (Join-Path $RunDirectory.FullName "ReviewSummary.txt") -Raw
$Failures = @()
if ($HandshakeText -notmatch 'handshake:\s+PASS' -or $HandshakeText -notmatch 'Successful joins:\s+4') {
	$Failures += "The packaged server did not accept all four clients."
}
if ($ReviewText -notmatch 'log review:\s+PASS' -or
	$ReviewText -notmatch 'Critical findings:\s+0' -or
	$ReviewText -notmatch 'GameplayCue overflow findings:\s+0') {
	$Failures += "The four-client log review did not pass cleanly."
}

$Result = if ($Failures.Count -eq 0) { "PASS" } else { "FAIL" }
$Summary = @(
	"Breachborne packaged four-client handshake: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Successful joins: 4/4",
	"Scope: transport connection only; 2v2 lobby/combat remains manual",
	"Evidence: $($RunDirectory.FullName)"
)
if ($Failures.Count -gt 0) {
	$Summary += "Failures:"
	$Summary += $Failures | ForEach-Object { "- $_" }
}
$Summary | Set-Content -LiteralPath (Join-Path $RunDirectory.FullName "FourClientSummary.txt") -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Failures.Count -gt 0) {
	exit 1
}
