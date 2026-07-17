param(
	[switch]$Strict
)

$ErrorActionPreference = "Stop"

$Root = (Resolve-Path "$PSScriptRoot\..\..").ProviderPath
$BinaryExtensions = [Collections.Generic.HashSet[string]]::new(
	[StringComparer]::OrdinalIgnoreCase)
@(
	".uasset", ".umap", ".png", ".jpg", ".jpeg", ".tga", ".exr", ".hdr", ".psd",
	".wav", ".mp3", ".ogg", ".fbx", ".obj", ".blend", ".ma", ".mb",
	".mp4", ".mov", ".avi", ".dll"
) | ForEach-Object { [void]$BinaryExtensions.Add($_) }

function Get-ScopeCategory([string]$Path) {
	if ($Path.StartsWith("Content/", [StringComparison]::OrdinalIgnoreCase)) {
		return "Project content"
	}
	if ($Path.StartsWith("Plugins/LudusAI/", [StringComparison]::OrdinalIgnoreCase)) {
		return "Ludus plugin"
	}
	if ($Path.StartsWith("For Ludus/", [StringComparison]::OrdinalIgnoreCase)) {
		return "Authoring references"
	}
	if ($Path.StartsWith("Source/", [StringComparison]::OrdinalIgnoreCase) -or
		$Path.StartsWith("Scripts/", [StringComparison]::OrdinalIgnoreCase) -or
		$Path.StartsWith("Config/", [StringComparison]::OrdinalIgnoreCase) -or
		$Path.StartsWith("docs/", [StringComparison]::OrdinalIgnoreCase) -or
		$Path.StartsWith(".vscode/", [StringComparison]::OrdinalIgnoreCase) -or
		$Path -in @(".gitattributes", ".gitignore")) {
		return "Source and tooling"
	}
	return "Other"
}

Push-Location $Root
try {
	$RawStatus = & git -c core.quotepath=false status --porcelain=v1 -z --untracked-files=all
	if ($LASTEXITCODE -ne 0) {
		throw "Could not enumerate pending files."
	}

	$StatusRecords = @([string]$RawStatus -split "`0" | Where-Object { $_.Length -gt 0 })
	$Pending = @(
		for ($RecordIndex = 0; $RecordIndex -lt $StatusRecords.Count; ++$RecordIndex) {
			$Record = $StatusRecords[$RecordIndex]
			if ($Record.Length -lt 4) {
				continue
			}

			$Status = $Record.Substring(0, 2)
			$Path = $Record.Substring(3)
			$OriginalPath = $null
			if ($Status.Contains("R") -or $Status.Contains("C")) {
				++$RecordIndex
				if ($RecordIndex -lt $StatusRecords.Count) {
					$OriginalPath = $StatusRecords[$RecordIndex]
				}
			}

			$FullPath = Join-Path $Root $Path
			$Exists = Test-Path -LiteralPath $FullPath -PathType Leaf
			$Extension = [IO.Path]::GetExtension($Path)
			$IsBinary = $BinaryExtensions.Contains($Extension)
			$UsesLfs = $false
			if ($IsBinary -and $Exists) {
				$Attribute = & git check-attr filter -- $Path
				$UsesLfs = $LASTEXITCODE -eq 0 -and $Attribute -match ": filter: lfs$"
			}

			[pscustomobject]@{
				Status = $Status
				Category = Get-ScopeCategory $Path
				Path = $Path
				OriginalPath = $OriginalPath
				Exists = $Exists
				Bytes = if ($Exists) { (Get-Item -LiteralPath $FullPath).Length } else { 0 }
				IsBinary = $IsBinary
				UsesLfs = $UsesLfs
				StaleIndex = $Status[0] -notin @(" ", "?") -and $Status[1] -ne " "
			}
		}
	)

	$StaleIndex = @($Pending | Where-Object StaleIndex)
	$BinaryAttributeFailures = @($Pending | Where-Object { $_.IsBinary -and $_.Bytes -gt 0 -and -not $_.UsesLfs })

	Write-Host "Breachborne pending commit scope"
	Write-Host "Pending paths: $($Pending.Count)"
	Write-Host "Staged then modified paths: $($StaleIndex.Count)"
	Write-Host "Pending binary paths without LFS attributes: $($BinaryAttributeFailures.Count)"
	foreach ($Category in @("Source and tooling", "Project content", "Ludus plugin", "Authoring references", "Other")) {
		$Entries = @($Pending | Where-Object Category -eq $Category)
		if ($Entries.Count -eq 0) {
			continue
		}

		Write-Host ""
		Write-Host "$Category ($($Entries.Count))"
		$Entries | Sort-Object Path | ForEach-Object {
			$Flags = @()
			if ($_.StaleIndex) { $Flags += "STALE_INDEX" }
			if ($_.IsBinary -and $_.Exists) { $Flags += $(if ($_.UsesLfs) { "LFS" } else { "NON_LFS" }) }
			$Size = if ($_.Exists) { "{0:N2} MiB" -f ($_.Bytes / 1MB) } else { "deleted" }
			$Suffix = if ($Flags.Count -gt 0) { " [$($Flags -join ',')]" } else { "" }
			Write-Host "- $($_.Status) $Size $($_.Path)$Suffix"
		}
	}

	if ($Strict -and ($StaleIndex.Count -gt 0 -or $BinaryAttributeFailures.Count -gt 0)) {
		exit 1
	}
}
finally {
	Pop-Location
}

$global:LASTEXITCODE = 0
