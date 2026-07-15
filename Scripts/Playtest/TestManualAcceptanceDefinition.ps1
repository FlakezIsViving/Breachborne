param(
	[string]$DefinitionPath = "$PSScriptRoot\ManualAcceptanceDefinition.psd1",
	[string]$ManualChecklistPath = "$PSScriptRoot\..\..\docs\plans\july-31-manual-vfx-acceptance.md",
	[string]$RangeChecklistPath = "$PSScriptRoot\..\..\docs\ABILITY_RANGE_INDICATORS.md"
)

$ErrorActionPreference = "Stop"

$Definitions = Import-PowerShellDataFile -LiteralPath $DefinitionPath
$Expected = [ordered]@{
	GhostEluna = @{ Prefix = "GE"; Count = 12 }
	KingpinHudson = @{ Prefix = "KH"; Count = 12 }
	CrystaVoid = @{ Prefix = "CV"; Count = 12 }
	CleanupStress = @{ Prefix = "CS"; Count = 5 }
	RangeIndicators = @{ Prefix = "RI"; Count = 9 }
	WispUI = @{ Prefix = "WI"; Count = 5 }
}

$Failures = @()
$Rows = @()
foreach ($BatchName in $Expected.Keys) {
	$BatchRows = @($Definitions[$BatchName])
	if ($BatchRows.Count -ne $Expected[$BatchName].Count) {
		$Failures += "$BatchName expected $($Expected[$BatchName].Count) rows, found $($BatchRows.Count)."
	}
	foreach ($Row in $BatchRows) {
		if ([string]::IsNullOrWhiteSpace([string]$Row.Description)) {
			$Failures += "$BatchName row '$($Row.Id)' has no description."
		}
		if ([string]$Row.Id -notmatch "^$($Expected[$BatchName].Prefix)-\d{2}$") {
			$Failures += "$BatchName has invalid row ID '$($Row.Id)'."
		}
		$Rows += $Row
	}
}

$DefinitionIds = @($Rows | ForEach-Object { [string]$_.Id })
$UniqueDefinitionIds = @($DefinitionIds | Sort-Object -Unique)
if ($DefinitionIds.Count -ne 55 -or $UniqueDefinitionIds.Count -ne 55) {
	$Failures += "Structured definition must contain 55 unique IDs; total=$($DefinitionIds.Count), unique=$($UniqueDefinitionIds.Count)."
}

$ChecklistText = (Get-Content -LiteralPath $ManualChecklistPath -Raw) + "`n" +
	(Get-Content -LiteralPath $RangeChecklistPath -Raw)
$DocumentedIds = @([regex]::Matches(
	$ChecklistText,
	'(?<![A-Z0-9-])(GE|KH|CV|CS|RI|WI)-\d{2}(?![A-Z0-9-])') | ForEach-Object Value)
$UniqueDocumentedIds = @($DocumentedIds | Sort-Object -Unique)
if ($DocumentedIds.Count -ne 55 -or $UniqueDocumentedIds.Count -ne 55) {
	$Failures += "Documentation must contain each of the 55 IDs exactly once; total=$($DocumentedIds.Count), unique=$($UniqueDocumentedIds.Count)."
}

$Differences = @(Compare-Object `
	-ReferenceObject @($UniqueDefinitionIds | Sort-Object) `
	-DifferenceObject @($UniqueDocumentedIds | Sort-Object))
foreach ($Difference in $Differences) {
	$Failures += "Definition/document mismatch: $($Difference.InputObject) $($Difference.SideIndicator)"
}

if ($Failures.Count) {
	Write-Host "Manual acceptance definition: FAIL"
	$Failures | ForEach-Object { Write-Host "- $_" }
	exit 1
}

Write-Host "Manual acceptance definition: PASS"
Write-Host "  Batches: $($Expected.Count)"
Write-Host "  Rows:    $($DefinitionIds.Count)/55 unique"
Write-Host "  Docs:    $($DocumentedIds.Count)/55 exact matches"
$global:LASTEXITCODE = 0
