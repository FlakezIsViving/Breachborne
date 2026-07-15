param(
	[string]$BundleRoot = "$PSScriptRoot\..\.."
)

$ErrorActionPreference = "Stop"

$Root = (Resolve-Path -LiteralPath $BundleRoot).ProviderPath
$RootPrefix = $Root.TrimEnd([IO.Path]::DirectorySeparatorChar, [IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
$ManifestPath = Join-Path $Root "SHA256SUMS.txt"
if (-not (Test-Path -LiteralPath $ManifestPath)) {
	throw "Distribution manifest not found: $ManifestPath"
}

$Checked = 0
$Failures = @()
$ManifestFiles = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($Line in Get-Content -LiteralPath $ManifestPath) {
	if ([string]::IsNullOrWhiteSpace($Line)) {
		continue
	}
	if ($Line -notmatch '^([A-Fa-f0-9]{64})  (.+)$') {
		$Failures += "Malformed manifest line: $Line"
		continue
	}

	$ExpectedHash = $Matches[1].ToUpperInvariant()
	$RelativePath = $Matches[2].Replace('/', [IO.Path]::DirectorySeparatorChar)
	if ([IO.Path]::IsPathRooted($RelativePath)) {
		$Failures += "Rooted path is not allowed: $RelativePath"
		continue
	}

	$FilePath = [IO.Path]::GetFullPath((Join-Path $Root $RelativePath))
	if (-not $FilePath.StartsWith($RootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
		$Failures += "Path escapes bundle: $RelativePath"
		continue
	}
	if (-not $ManifestFiles.Add($FilePath)) {
		$Failures += "Duplicate manifest path: $RelativePath"
		continue
	}
	if (-not (Test-Path -LiteralPath $FilePath -PathType Leaf)) {
		$Failures += "Missing: $RelativePath"
		continue
	}

	$ActualHash = (Get-FileHash -LiteralPath $FilePath -Algorithm SHA256).Hash.ToUpperInvariant()
	if ($ActualHash -ne $ExpectedHash) {
		$Failures += "Hash mismatch: $RelativePath expected=$ExpectedHash actual=$ActualHash"
	}
	$Checked++
}

$AllowedUntrackedFiles = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
[void]$AllowedUntrackedFiles.Add($ManifestPath)
[void]$AllowedUntrackedFiles.Add((Join-Path $Root "DistributionVerification.txt"))
foreach ($File in Get-ChildItem -LiteralPath $Root -Recurse -File) {
	if (-not $ManifestFiles.Contains($File.FullName) -and -not $AllowedUntrackedFiles.Contains($File.FullName)) {
		$UnexpectedPath = if ($File.FullName.StartsWith($RootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
			$File.FullName.Substring($RootPrefix.Length)
		} else { $File.FullName }
		$Failures += "Unexpected file not covered by manifest: $UnexpectedPath"
	}
}

$Result = if ($Failures.Count -eq 0 -and $Checked -gt 0) { "PASS" } else { "FAIL" }
$Summary = @(
	"Breachborne playtest distribution verification: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Bundle: $Root",
	"Files checked: $Checked",
	"Failures: $($Failures.Count)"
)
if ($Failures.Count) {
	$Summary += $Failures | ForEach-Object { "- $_" }
}
$Summary | Set-Content -LiteralPath (Join-Path $Root "DistributionVerification.txt") -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Result -ne "PASS") {
	exit 1
}

$global:LASTEXITCODE = 0
