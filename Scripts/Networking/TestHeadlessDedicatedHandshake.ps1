param(
	[string]$ProjectPath = "$PSScriptRoot\..\..\Breachborne.uproject",
	[string]$Map = "/Game/Maps/TestMap",
	[int]$Port = 7787,
	[int]$ClientCount = 2,
	[int]$ServerStartupSeconds = 5,
	[int]$ConnectionSeconds = 20,
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\DedicatedHandshake",
	[string]$ServerExePath = "",
	[string]$ClientExePath = "",
	[switch]$Packaged,
	[switch]$AbilitySmoke,
	[int[]]$SmokeHunterIDs = @(),
	[string[]]$ServerExtraArgs = @(),
	[string[]]$ClientExtraArgs = @()
)

$ErrorActionPreference = "Stop"

if ($AbilitySmoke -and $SmokeHunterIDs.Count -ne $ClientCount) {
	throw "AbilitySmoke requires one SmokeHunterIDs entry per client."
}

$ResolvedProject = (Resolve-Path -LiteralPath $ProjectPath).ProviderPath
$ProjectRoot = Split-Path -Parent $ResolvedProject
$ServerExe = if ([string]::IsNullOrWhiteSpace($ServerExePath)) {
	Join-Path $ProjectRoot "Binaries\Win64\BreachborneServer.exe"
} else {
	$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ServerExePath)
}
$ClientExe = if ([string]::IsNullOrWhiteSpace($ClientExePath)) {
	Join-Path $ProjectRoot "Binaries\Win64\Breachborne.exe"
} else {
	$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($ClientExePath)
}

foreach ($Exe in @($ServerExe, $ClientExe)) {
	if (-not (Test-Path -LiteralPath $Exe)) {
		throw "Required executable not found: $Exe. Build Game and Server with the same source-engine toolchain first."
	}
}

if (Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue) {
	throw "UDP port $Port is already in use. Pass a different -Port value."
}

$Timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$RunDirectory = Join-Path $OutputRoot $Timestamp
New-Item -ItemType Directory -Force -Path $RunDirectory | Out-Null

$SpawnedProcesses = @()
$EarlyExitDescriptions = @()

try {
	$ServerArgs = @(
		$Map,
		"-port=$Port",
		"-unattended",
		"-nosteam",
		"-NoSound",
		"-stdout",
		"-FullStdOutLogOutput"
	)
	if (-not $Packaged) {
		$ServerArgs = @("`"$ResolvedProject`"") + $ServerArgs
	}
	if ($AbilitySmoke) {
		$ServerArgs += "-BBAbilitySmoke"
	}
	$ServerArgs += $ServerExtraArgs
	$Server = Start-Process -FilePath $ServerExe -ArgumentList $ServerArgs -WindowStyle Hidden `
		-RedirectStandardOutput (Join-Path $RunDirectory "Server.log") `
		-RedirectStandardError (Join-Path $RunDirectory "Server.err") `
		-PassThru
	$SpawnedProcesses += $Server

	Start-Sleep -Seconds $ServerStartupSeconds
	if ($Server.HasExited) {
		throw "Dedicated server exited during startup with code $($Server.ExitCode)."
	}

	for ($Index = 1; $Index -le $ClientCount; $Index++) {
		$ClientArgs = @(
			"127.0.0.1:$Port",
			"-game",
			"-nullrhi",
			"-unattended",
			"-nosteam",
			"-NoSound",
			"-stdout",
			"-FullStdOutLogOutput"
		)
		if (-not $Packaged) {
			$ClientArgs = @("`"$ResolvedProject`"") + $ClientArgs
		}
		if ($AbilitySmoke) {
			$ClientArgs += @(
				"-BBAbilitySmoke",
				"-BBAbilitySmokeIndex=$Index",
				"-BBAbilitySmokeHunter=$($SmokeHunterIDs[$Index - 1])"
			)
		}
		$ClientArgs += $ClientExtraArgs
		$Client = Start-Process -FilePath $ClientExe -ArgumentList $ClientArgs -WindowStyle Hidden `
			-RedirectStandardOutput (Join-Path $RunDirectory "Client$Index.log") `
			-RedirectStandardError (Join-Path $RunDirectory "Client$Index.err") `
			-PassThru
		$SpawnedProcesses += $Client
	}

	Start-Sleep -Seconds $ConnectionSeconds
	foreach ($Process in $SpawnedProcesses) {
		if ($Process.HasExited) {
			$EarlyExitDescriptions += "$($Process.ProcessName) PID $($Process.Id) exited with code $($Process.ExitCode)"
		}
	}
}
finally {
	foreach ($Process in $SpawnedProcesses) {
		if ($Process -and -not $Process.HasExited) {
			Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue
		}
	}
}

$ServerLogPath = Join-Path $RunDirectory "Server.log"
$ServerText = Get-Content -LiteralPath $ServerLogPath -Raw
$JoinCount = ([regex]::Matches($ServerText, "Join succeeded:")).Count
$FailurePattern = "OutdatedClient|Network Failure|Travel Failure|Connection TIMED OUT|Fatal error|Ensure condition failed"
$Failures = @()

if ($ServerText -match $FailurePattern) {
	$Failures += "Server log contains a network/fatal failure."
}
if ($JoinCount -ne $ClientCount) {
	$Failures += "Expected $ClientCount successful server joins; observed $JoinCount."
}

for ($Index = 1; $Index -le $ClientCount; $Index++) {
	$ClientLogPath = Join-Path $RunDirectory "Client$Index.log"
	$ClientText = Get-Content -LiteralPath $ClientLogPath -Raw
	if ($ClientText -notmatch "Welcomed by server") {
		$Failures += "Client $Index was not welcomed by the server."
	}
	if ($ClientText -match $FailurePattern) {
		$Failures += "Client $Index log contains a network/fatal failure."
	}
}

$Failures += $EarlyExitDescriptions
$Result = if ($Failures.Count -eq 0) { "PASS" } else { "FAIL" }
$Summary = @(
	"Breachborne headless dedicated handshake: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Map: $Map",
	"Port: $Port",
	"Expected clients: $ClientCount",
	"Successful joins: $JoinCount",
	"Mode: $(if ($Packaged) { 'Packaged' } else { 'Local binaries' })",
	"Server extra args: $($ServerExtraArgs -join ' ')",
	"Client extra args: $($ClientExtraArgs -join ' ')",
	"Server executable: $ServerExe",
	"Client executable: $ClientExe",
	"Evidence: $RunDirectory"
)

if ($Failures.Count -gt 0) {
	$Summary += "Failures:"
	$Summary += $Failures | ForEach-Object { "- $_" }
}

$SummaryPath = Join-Path $RunDirectory "Summary.txt"
$Summary | Set-Content -LiteralPath $SummaryPath -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Failures.Count -gt 0) {
	exit 1
}
