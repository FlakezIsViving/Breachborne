param(
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Tests\InteractivePlaytestReviewer"
)

$ErrorActionPreference = "Stop"

$ResolvedOutputRoot = [IO.Path]::GetFullPath($OutputRoot)
$RunRoot = Join-Path $ResolvedOutputRoot ([guid]::NewGuid().ToString("N"))
$SessionDirectory = Join-Path $RunRoot "CentralSession"
$ClientBinDirectory = Join-Path $RunRoot "Client\Binaries\Win64"
$ServerBinDirectory = Join-Path $RunRoot "Server\Binaries\Win64"
$RelativeLogRoot = ".\Saved\Logs\InteractivePlaytest\Synthetic"

New-Item -ItemType Directory -Force -Path $SessionDirectory | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $ClientBinDirectory $RelativeLogRoot) | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $ServerBinDirectory $RelativeLogRoot) | Out-Null

$ClientLog = Join-Path $ClientBinDirectory (Join-Path $RelativeLogRoot "Client1.log")
$ServerLog = Join-Path $ServerBinDirectory (Join-Path $RelativeLogRoot "Server.log")
"LogNet: Join succeeded: synthetic" | Set-Content -LiteralPath $ServerLog -Encoding UTF8
"LogBreachborne: BB_NETFLOW|CLIENT|Synthetic" | Set-Content -LiteralPath $ClientLog -Encoding UTF8

$Session = [ordered]@{
	StartedAt = (Get-Date -Format o)
	SessionLabel = "interactive reviewer recovery fixture"
	RunDirectory = $SessionDirectory
	ClientCount = 1
	Processes = @(
		[ordered]@{
			Role = "Server"
			PID = 0
			Executable = (Join-Path $ServerBinDirectory "BreachborneServer.exe")
			Log = (Join-Path $RelativeLogRoot "Server.log")
		},
		[ordered]@{
			Role = "Client1"
			PID = 0
			Executable = (Join-Path $ClientBinDirectory "Breachborne.exe")
			Log = (Join-Path $RelativeLogRoot "Client1.log")
		}
	)
}
$Session | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $SessionDirectory "Session.json") -Encoding UTF8

try {
	$global:LASTEXITCODE = 0
	& (Join-Path $PSScriptRoot "ReviewInteractivePlaytest.ps1") -SessionDirectory $SessionDirectory
	if ($LASTEXITCODE -ne 0) {
		throw "Synthetic interactive review failed with exit code $LASTEXITCODE."
	}

	foreach ($LogName in "Server.log", "Client1.log") {
		if (-not (Test-Path -LiteralPath (Join-Path $SessionDirectory $LogName))) {
			throw "Reviewer did not recover $LogName into the central session."
		}
	}

	$Summary = Get-Content -LiteralPath (Join-Path $SessionDirectory "ReviewSummary.txt") -Raw
	if ($Summary -notmatch "interactive packaged log review: PASS" -or $Summary -notmatch "Recovered logs: 2") {
		throw "Reviewer summary did not record a passing two-log recovery."
	}

	Write-Host "Interactive playtest reviewer recovery: PASS"
}
finally {
	if (Test-Path -LiteralPath $RunRoot) {
		$OutputPrefix = $ResolvedOutputRoot.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
		$ResolvedRunRoot = (Resolve-Path -LiteralPath $RunRoot).ProviderPath
		if (-not $ResolvedRunRoot.StartsWith($OutputPrefix, [StringComparison]::OrdinalIgnoreCase)) {
			throw "Refusing to remove synthetic fixture outside $ResolvedOutputRoot`: $ResolvedRunRoot"
		}
		Remove-Item -LiteralPath $ResolvedRunRoot -Recurse -Force
	}
}

$global:LASTEXITCODE = 0
