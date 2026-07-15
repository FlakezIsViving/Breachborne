param(
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\DirectIPReviewerSelfTest"
)

$ErrorActionPreference = "Stop"

$ResolvedOutputRoot = [IO.Path]::GetFullPath($OutputRoot)
New-Item -ItemType Directory -Force -Path $ResolvedOutputRoot | Out-Null
$RunDirectory = Join-Path $ResolvedOutputRoot ([guid]::NewGuid().ToString("N"))
$ServerDirectory = Join-Path $RunDirectory "Server"
$HostDirectory = Join-Path $RunDirectory "Host"
$RemoteInitialDirectory = Join-Path $RunDirectory "RemoteInitial"
$RemoteReconnectDirectory = Join-Path $RunDirectory "RemoteReconnect"
New-Item -ItemType Directory -Force -Path @(
	$ServerDirectory,
	$HostDirectory,
	$RemoteInitialDirectory,
	$RemoteReconnectDirectory) | Out-Null

$Reviewer = Join-Path $PSScriptRoot "ReviewDirectIPPlaytest.ps1"
$ServerLog = Join-Path $ServerDirectory "Server.log"
$HostLog = Join-Path $HostDirectory "Client.log"
$RemoteInitialLog = Join-Path $RemoteInitialDirectory "Client.log"
$RemoteReconnectLog = Join-Path $RemoteReconnectDirectory "Client.log"
$PassedScenarios = 0

$ValidServerLines = @(
	"LogNet: Join succeeded: HostPlayer",
	"LogNet: Join succeeded: RemotePlayer",
	"LogNet: Join succeeded: RemoteReconnect",
	"LogBreachborne: BB_RECONNECT|SERVER|Restored player=RemoteReconnect team=1 slot=0 hunter=2"
)
$ValidClientLines = @(
	"LogNet: Welcomed by server (Level: /Game/Maps/TestMap)",
	"LogBreachborne: BB_NETFLOW|CLIENT|EnterGameplayPhase"
)

function Write-RemoteSession([string]$Directory, [string]$Address) {
	[ordered]@{
		StartedAt = (Get-Date -Format o)
		Address = $Address
		PID = 1
		Executable = "Breachborne.exe"
		ExecutableSHA256 = "0123456789ABCDEF"
		ExtraArgs = @()
		Log = (Join-Path $Directory "Client.log")
	} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $Directory "Session.json") -Encoding UTF8
}

function Restore-ValidFixture {
	$ValidServerLines | Set-Content -LiteralPath $ServerLog -Encoding UTF8
	$ValidClientLines | Set-Content -LiteralPath $HostLog -Encoding UTF8
	$ValidClientLines | Set-Content -LiteralPath $RemoteInitialLog -Encoding UTF8
	$ValidClientLines | Set-Content -LiteralPath $RemoteReconnectLog -Encoding UTF8
	Write-RemoteSession $RemoteInitialDirectory "192.0.2.10:7777"
	Write-RemoteSession $RemoteReconnectDirectory "192.0.2.10:7777"
}

function Invoke-ExpectedResult([string]$Scenario, [bool]$ShouldPass) {
	$global:LASTEXITCODE = 0
	& $Reviewer `
		-ServerDirectory $ServerDirectory `
		-HostClientDirectory $HostDirectory `
		-RemoteClientDirectories $RemoteInitialDirectory,$RemoteReconnectDirectory *> $null
	$Passed = $LASTEXITCODE -eq 0
	if ($Passed -ne $ShouldPass) {
		throw "Direct-IP reviewer scenario '$Scenario' expected pass=$ShouldPass but observed pass=$Passed."
	}
	$script:PassedScenarios++
}

try {
	Restore-ValidFixture
	Invoke-ExpectedResult "valid three-join reconnect" $true

	$ValidClientLines + "Fatal error: synthetic" | Set-Content -LiteralPath $RemoteReconnectLog -Encoding UTF8
	Invoke-ExpectedResult "critical remote log" $false

	Restore-ValidFixture
	Write-RemoteSession $RemoteReconnectDirectory "127.0.0.1:7777"
	Invoke-ExpectedResult "remote loopback mismatch" $false

	Restore-ValidFixture
	$ValidServerLines[0..2] | Set-Content -LiteralPath $ServerLog -Encoding UTF8
	Invoke-ExpectedResult "missing reconnect restoration" $false

	$DirectIPDoc = Get-Content -LiteralPath (Join-Path $PSScriptRoot "..\..\docs\plans\july-31-direct-ip-acceptance.md") -Raw
	$DirectIPIds = @([regex]::Matches($DirectIPDoc, '(?<![A-Z0-9-])DI-\d{2}(?![A-Z0-9-])') |
		ForEach-Object Value)
	if ($DirectIPIds.Count -ne 6 -or @($DirectIPIds | Sort-Object -Unique).Count -ne 6) {
		throw "Direct-IP acceptance document must contain six unique DI row IDs exactly once."
	}
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

Write-Host "Direct-IP reviewer self-test: PASS"
Write-Host "  Scenarios: $PassedScenarios/4"
Write-Host "  Acceptance IDs: 6/6 unique"
Write-Host "  Temporary fixture removed: $RunDirectory"
$global:LASTEXITCODE = 0
