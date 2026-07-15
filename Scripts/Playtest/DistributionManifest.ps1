function Write-BBDistributionManifest {
	[CmdletBinding()]
	param(
		[Parameter(Mandatory = $true)]
		[string]$BundleRoot
	)

	$ResolvedBundleRoot = (Resolve-Path -LiteralPath $BundleRoot).ProviderPath
	$BundlePrefix = $ResolvedBundleRoot.TrimEnd(
		[IO.Path]::DirectorySeparatorChar,
		[IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
	$ManifestPath = Join-Path $ResolvedBundleRoot "SHA256SUMS.txt"
	$VerificationPath = Join-Path $ResolvedBundleRoot "DistributionVerification.txt"
	$Files = @(Get-ChildItem -LiteralPath $ResolvedBundleRoot -Recurse -File |
		Where-Object { $_.FullName -ne $ManifestPath -and $_.FullName -ne $VerificationPath } |
		Sort-Object FullName)

	$Lines = foreach ($File in $Files) {
		if (-not $File.FullName.StartsWith($BundlePrefix, [StringComparison]::OrdinalIgnoreCase)) {
			throw "Refusing to hash a file outside the bundle root: $($File.FullName)"
		}
		$RelativePath = $File.FullName.Substring($BundlePrefix.Length).Replace('\', '/')
		"$((Get-FileHash -LiteralPath $File.FullName -Algorithm SHA256).Hash)  $RelativePath"
	}
	$Lines | Set-Content -LiteralPath $ManifestPath -Encoding ASCII
	return $ManifestPath
}
