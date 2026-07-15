param(
	[Parameter(Mandatory = $true)]
	[string]$SessionDirectory,
	[Parameter(Mandatory = $true)]
	[ValidateSet("GhostEluna", "KingpinHudson", "CrystaVoid", "CleanupStress", "RangeIndicators", "WispUI")]
	[string]$Batch,
	[switch]$Force,
	[switch]$ValidateOnly
)

$ErrorActionPreference = "Stop"

$ResolvedSession = (Resolve-Path -LiteralPath $SessionDirectory).ProviderPath
$Definitions = Import-PowerShellDataFile -LiteralPath (Join-Path $PSScriptRoot "ManualAcceptanceDefinition.psd1")
$Rows = @($Definitions[$Batch])
if (-not $Rows.Count) {
	throw "No manual acceptance rows are defined for batch '$Batch'."
}

$RecordPath = Join-Path $ResolvedSession "$Batch-ManualAcceptance.md"
if ((Test-Path -LiteralPath $RecordPath) -and -not $Force) {
	throw "Acceptance record already exists: $RecordPath. Use -Force only to replace an unused record."
}

$Lines = @(
	"# $Batch Manual Acceptance",
	"",
	"Session: ``$ResolvedSession``  ",
	"Created: $(Get-Date -Format o)  ",
	"Master criteria: ``docs/plans/july-31-manual-vfx-acceptance.md``",
	"",
	"Use ``PASS``, ``FAIL``, or ``SETUP BLOCKED`` only after filling the observation columns. A",
	"clean log review does not complete these rows.",
	"",
	"| ID | Acceptance row | Owner | Observer | Gameplay | Cleanup | Result | Notes |",
	"| --- | --- | --- | --- | --- | --- | --- | --- |"
)
foreach ($Row in $Rows) {
	$Lines += "| $($Row.Id) | $($Row.Description) | [ ] | [ ] | [ ] | [ ] | PENDING | |"
}
$Lines += @(
	"",
	"## Session Review",
	"",
	"- Log review: PENDING",
	"- Review summary: ``$ResolvedSession\ReviewSummary.txt``",
	"- Critical findings: PENDING",
	"- GameplayCue overflow findings: PENDING",
	"- Follow-up issue IDs: none recorded"
)

if ($ValidateOnly) {
	Write-Host "Manual acceptance record validation: PASS"
	Write-Host "  Batch: $Batch"
	Write-Host "  Rows:  $($Rows.Count)"
	Write-Host "  Target: $RecordPath"
	Write-Host "No record was written."
	$global:LASTEXITCODE = 0
	return
}

$Lines | Set-Content -LiteralPath $RecordPath -Encoding UTF8
Write-Host "Manual acceptance record: $RecordPath"
$global:LASTEXITCODE = 0
