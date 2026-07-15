param(
	[long]$LargeFileThresholdBytes = 5MB
)

$ErrorActionPreference = "Stop"

$Root = (Resolve-Path "$PSScriptRoot\..\..").ProviderPath
Push-Location $Root

try {
	& git --version | Out-Null
	if ($LASTEXITCODE -ne 0) {
		throw "Git is unavailable."
	}

	& git lfs version | Out-Null
	if ($LASTEXITCODE -ne 0) {
		throw "Git LFS is unavailable."
	}

	$TrackedPaths = @(& git -c core.quotepath=false ls-files)
	if ($LASTEXITCODE -ne 0) {
		throw "Could not enumerate tracked files."
	}

	$LfsPaths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
	@(& git -c core.quotepath=false lfs ls-files -n) | ForEach-Object {
		if (-not [string]::IsNullOrWhiteSpace($_)) {
			[void]$LfsPaths.Add($_)
		}
	}
	if ($LASTEXITCODE -ne 0) {
		throw "Could not enumerate Git LFS files."
	}

	$LargeFiles = @(
		foreach ($RelativePath in $TrackedPaths) {
			if ([string]::IsNullOrWhiteSpace($RelativePath)) {
				continue
			}

			$FullPath = Join-Path $Root $RelativePath
			if (-not (Test-Path -LiteralPath $FullPath -PathType Leaf)) {
				continue
			}

			$File = Get-Item -LiteralPath $FullPath
			if ($File.Length -ge $LargeFileThresholdBytes) {
				[pscustomobject]@{
					Path = $RelativePath
					Bytes = $File.Length
					InLfs = $LfsPaths.Contains($RelativePath)
				}
			}
		}
	)

	$LargeFileViolations = @($LargeFiles | Where-Object { -not $_.InLfs })
	$RebuildablePaths = @("Binaries", "Intermediate", "Saved", "DerivedDataCache", "Builds", ".vs", ".vscode")
	$IgnoreFailures = @(
		foreach ($RelativePath in $RebuildablePaths) {
			& git check-ignore -q -- $RelativePath
			if ($LASTEXITCODE -ne 0) {
				$RelativePath
			}
		}
	)

	$LfsSamples = @(
		"Content/__BB_LFS_POLICY__.uasset",
		"Content/__BB_LFS_POLICY__.umap",
		"Characters/__BB_LFS_POLICY__.fbx",
		"Content/__BB_LFS_POLICY__.png",
		"Content/__BB_LFS_POLICY__.wav"
	)
	$AttributeFailures = @(
		foreach ($SamplePath in $LfsSamples) {
			$Attribute = & git check-attr filter -- $SamplePath
			if ($LASTEXITCODE -ne 0 -or $Attribute -notmatch ": filter: lfs$") {
				$SamplePath
			}
		}
	)

	Write-Host "Breachborne repository asset policy"
	Write-Host "Tracked files: $($TrackedPaths.Count)"
	Write-Host "Git LFS files: $($LfsPaths.Count)"
	Write-Host "Tracked files >= $([math]::Round($LargeFileThresholdBytes / 1MB, 2)) MiB: $($LargeFiles.Count)"
	Write-Host "Large files outside LFS: $($LargeFileViolations.Count)"
	Write-Host "Rebuildable ignore rules: $(if ($IgnoreFailures.Count -eq 0) { 'PASS' } else { 'FAIL' })"
	Write-Host "Binary asset LFS attributes: $(if ($AttributeFailures.Count -eq 0) { 'PASS' } else { 'FAIL' })"

	$LargeFileViolations | Sort-Object Bytes -Descending | ForEach-Object {
		Write-Host "- NON_LFS $([math]::Round($_.Bytes / 1MB, 2)) MiB $($_.Path)"
	}
	$IgnoreFailures | ForEach-Object { Write-Host "- NOT_IGNORED $_" }
	$AttributeFailures | ForEach-Object { Write-Host "- NOT_LFS_TRACKED $_" }

	if ($LargeFileViolations.Count -gt 0 -or $IgnoreFailures.Count -gt 0 -or $AttributeFailures.Count -gt 0) {
		exit 1
	}
}
finally {
	Pop-Location
}

$global:LASTEXITCODE = 0
