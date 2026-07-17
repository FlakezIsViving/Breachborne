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
	$TrackedPathSet = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
	$TrackedPaths | ForEach-Object { [void]$TrackedPathSet.Add($_) }

	$RawStatus = & git -c core.quotepath=false status --porcelain=v1 -z --untracked-files=all
	if ($LASTEXITCODE -ne 0) {
		throw "Could not enumerate pending files."
	}
	$StatusRecords = @([string]$RawStatus -split "`0" | Where-Object { $_.Length -gt 0 })
	$PendingPaths = @(
		for ($RecordIndex = 0; $RecordIndex -lt $StatusRecords.Count; ++$RecordIndex) {
			$Record = $StatusRecords[$RecordIndex]
			if ($Record.Length -lt 4) {
				continue
			}

			$Status = $Record.Substring(0, 2)
			$Record.Substring(3)
			if ($Status.Contains("R") -or $Status.Contains("C")) {
				++$RecordIndex
			}
		}
	)
	$CandidatePaths = @($TrackedPaths + $PendingPaths | Sort-Object -Unique)

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
		foreach ($RelativePath in $CandidatePaths) {
			if ([string]::IsNullOrWhiteSpace($RelativePath)) {
				continue
			}

			$FullPath = Join-Path $Root $RelativePath
			if (-not (Test-Path -LiteralPath $FullPath -PathType Leaf)) {
				continue
			}

			$File = Get-Item -LiteralPath $FullPath
			if ($File.Length -ge $LargeFileThresholdBytes) {
				$FilterAttribute = & git check-attr filter -- $RelativePath
				$UsesLfsAttribute = $LASTEXITCODE -eq 0 -and $FilterAttribute -match ": filter: lfs$"
				[pscustomobject]@{
					Path = $RelativePath
					Bytes = $File.Length
					InLfs = if ($TrackedPathSet.Contains($RelativePath)) {
						$LfsPaths.Contains($RelativePath)
					} else {
						$UsesLfsAttribute
					}
				}
			}
		}
	)

	$LargeFileViolations = @($LargeFiles | Where-Object { -not $_.InLfs })
	$RebuildablePaths = @(
		"Binaries",
		"Intermediate",
		"Saved",
		"DerivedDataCache",
		"Builds",
		".vs",
		".vscode/compileCommands_Breachborne",
		".vscode/compileCommands_Default",
		".vscode/compileCommands_Breachborne.json",
		".vscode/compileCommands_Default.json",
		".vscode/c_cpp_properties.json"
	)
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

	$PackageRoots = @(
		Get-Item -LiteralPath (Join-Path $Root "Content") -ErrorAction SilentlyContinue
		Get-ChildItem -LiteralPath (Join-Path $Root "Plugins") -Directory -ErrorAction SilentlyContinue |
			ForEach-Object {
				Get-Item -LiteralPath (Join-Path $_.FullName "Content") -ErrorAction SilentlyContinue
			}
	)
	$NonAsciiPackagePaths = @(
		foreach ($PackageRoot in $PackageRoots) {
			Get-ChildItem -LiteralPath $PackageRoot.FullName -Recurse -File |
				Where-Object { $_.Extension -in ".uasset", ".umap" } |
				ForEach-Object {
					$RelativePath = $_.FullName.Substring($Root.Length + 1)
					$InvalidCharacter = $RelativePath.ToCharArray() |
						Where-Object { [int]$_ -lt 0x20 -or [int]$_ -gt 0x7e } |
						Select-Object -First 1
					if ($null -ne $InvalidCharacter) {
						$RelativePath
					}
				}
		}
	)

	Write-Host "Breachborne repository asset policy"
	Write-Host "Tracked files: $($TrackedPaths.Count)"
	Write-Host "Pending paths: $($PendingPaths.Count)"
	Write-Host "Git LFS files: $($LfsPaths.Count)"
	Write-Host "Tracked or pending files >= $([math]::Round($LargeFileThresholdBytes / 1MB, 2)) MiB: $($LargeFiles.Count)"
	Write-Host "Large files outside LFS: $($LargeFileViolations.Count)"
	Write-Host "Rebuildable ignore rules: $(if ($IgnoreFailures.Count -eq 0) { 'PASS' } else { 'FAIL' })"
	Write-Host "Binary asset LFS attributes: $(if ($AttributeFailures.Count -eq 0) { 'PASS' } else { 'FAIL' })"
	Write-Host "Printable ASCII package paths: $(if ($NonAsciiPackagePaths.Count -eq 0) { 'PASS' } else { 'FAIL' })"

	$LargeFileViolations | Sort-Object Bytes -Descending | ForEach-Object {
		Write-Host "- NON_LFS $([math]::Round($_.Bytes / 1MB, 2)) MiB $($_.Path)"
	}
	$IgnoreFailures | ForEach-Object { Write-Host "- NOT_IGNORED $_" }
	$AttributeFailures | ForEach-Object { Write-Host "- NOT_LFS_TRACKED $_" }
	$NonAsciiPackagePaths | ForEach-Object { Write-Host "- NON_ASCII_PACKAGE_PATH $_" }

	if ($LargeFileViolations.Count -gt 0 -or $IgnoreFailures.Count -gt 0 -or
		$AttributeFailures.Count -gt 0 -or $NonAsciiPackagePaths.Count -gt 0) {
		exit 1
	}
}
finally {
	Pop-Location
}

$global:LASTEXITCODE = 0
