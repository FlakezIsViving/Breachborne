param(
	[int]$Port = 7831,
	[int]$ServerStartupSeconds = 4,
	[int]$InitialMatchSeconds = 42,
	[int]$ReconnectSeconds = 12,
	[string]$ClientBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsClient",
	[string]$ServerBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsServer",
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\PackagedReconnect"
)

$ErrorActionPreference = "Stop"

if (Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue) {
	throw "UDP port $Port is already in use."
}

$ClientRoot = (Resolve-Path -LiteralPath $ClientBuildRoot).ProviderPath
$ServerRoot = (Resolve-Path -LiteralPath $ServerBuildRoot).ProviderPath
$ClientExe = Get-ChildItem -LiteralPath $ClientRoot -Recurse -Filter "Breachborne.exe" |
	Where-Object { $_.FullName -match "\\Binaries\\Win64\\" -and $_.FullName -notmatch "Server" } |
	Select-Object -First 1
$ServerExe = Get-ChildItem -LiteralPath $ServerRoot -Recurse -Filter "BreachborneServer.exe" |
	Where-Object { $_.FullName -match "\\Binaries\\Win64\\" } |
	Select-Object -First 1
if (-not $ClientExe -or -not $ServerExe) {
	throw "Packaged client/server executables were not found."
}

$RunDirectory = Join-Path $OutputRoot (Get-Date -Format "yyyyMMdd-HHmmss")
New-Item -ItemType Directory -Force -Path $RunDirectory | Out-Null
$Processes = @()

function Start-SmokeClient([int]$Index, [int]$HunterID, [string]$LogName) {
	$Arguments = @(
		"127.0.0.1:$Port", "-game", "-nullrhi", "-unattended", "-nosteam", "-NoSound",
		"-stdout", "-FullStdOutLogOutput", "-BBAbilitySmoke", "-BBAbilitySmokeIndex=$Index",
		"-BBAbilitySmokeHunter=$HunterID"
	)
	return Start-Process -FilePath $ClientExe.FullName -ArgumentList $Arguments -WindowStyle Hidden `
		-RedirectStandardOutput (Join-Path $RunDirectory $LogName) `
		-RedirectStandardError (Join-Path $RunDirectory ($LogName -replace '\.log$', '.err')) -PassThru
}

try {
	$ServerArguments = @(
		"/Game/Maps/TestMap", "-port=$Port", "-unattended", "-nosteam", "-NoSound", "-stdout",
		"-FullStdOutLogOutput", "-BBAbilitySmoke"
	)
	$Server = Start-Process -FilePath $ServerExe.FullName -ArgumentList $ServerArguments -WindowStyle Hidden `
		-RedirectStandardOutput (Join-Path $RunDirectory "Server.log") `
		-RedirectStandardError (Join-Path $RunDirectory "Server.err") -PassThru
	$Processes += $Server
	Start-Sleep -Seconds $ServerStartupSeconds
	if ($Server.HasExited) {
		throw "Packaged server exited during startup with code $($Server.ExitCode)."
	}

	$Client1 = Start-SmokeClient 1 1 "Client1.log"
	$Client2 = Start-SmokeClient 2 2 "Client2.log"
	$Processes += @($Client1, $Client2)
	Start-Sleep -Seconds $InitialMatchSeconds
	if ($Client1.HasExited -or $Client2.HasExited) {
		throw "An initial smoke client exited before the reconnect attempt."
	}

	Stop-Process -Id $Client2.Id -Force
	$Client2.WaitForExit()
	Start-Sleep -Seconds 2

	$ReconnectArguments = @(
		"127.0.0.1:$Port", "-game", "-nullrhi", "-unattended", "-nosteam", "-NoSound",
		"-stdout", "-FullStdOutLogOutput"
	)
	$ReconnectClient = Start-Process -FilePath $ClientExe.FullName -ArgumentList $ReconnectArguments `
		-WindowStyle Hidden -RedirectStandardOutput (Join-Path $RunDirectory "ReconnectClient.log") `
		-RedirectStandardError (Join-Path $RunDirectory "ReconnectClient.err") -PassThru
	$Processes += $ReconnectClient
	Start-Sleep -Seconds $ReconnectSeconds
}
finally {
	foreach ($Process in $Processes) {
		if ($Process -and -not $Process.HasExited) {
			Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue
		}
	}
}

$ServerText = Get-Content -LiteralPath (Join-Path $RunDirectory "Server.log") -Raw
$Client1Text = Get-Content -LiteralPath (Join-Path $RunDirectory "Client1.log") -Raw
$Client2Text = Get-Content -LiteralPath (Join-Path $RunDirectory "Client2.log") -Raw
$ReconnectText = Get-Content -LiteralPath (Join-Path $RunDirectory "ReconnectClient.log") -Raw
$FailurePattern = "Fatal error|Critical error|Ensure condition failed|Assertion failed|OutdatedClient|Network Failure|Travel Failure|Connection TIMED OUT"
$Failures = @()
$JoinCount = ([regex]::Matches($ServerText, "Join succeeded:")).Count

if ($JoinCount -ne 3) {
	$Failures += "Expected three successful joins including the replacement client; observed $JoinCount."
}
if ($ServerText -notmatch "BB_NETFLOW\|SERVER\|StartGameplayMatch players=2") {
	$Failures += "The initial two-player match did not reach gameplay."
}
if ($Client1Text -notmatch "BB_ABILITY_SMOKE\|COMPLETE\|index=1 hunter=1" -or
	$Client2Text -notmatch "BB_ABILITY_SMOKE\|COMPLETE\|index=2 hunter=2") {
	$Failures += "The initial Ghost/Kingpin ability smoke did not complete before disconnect."
}
if ($ReconnectText -notmatch "Welcomed by server") {
	$Failures += "The replacement client was not welcomed by the server."
}
if ($ServerText -notmatch "BB_LOBBY\|SERVER\|LateJoinSpectator player=.* phase=4") {
	$Failures += "The replacement client was not recorded as a Playing-phase spectator."
}
foreach ($Text in @($ServerText, $Client1Text, $Client2Text, $ReconnectText)) {
	if ($Text -match $FailurePattern) {
		$Failures += "A reconnect log contains a critical network/runtime failure."
		break
	}
}

$Result = if ($Failures.Count -eq 0) { "PASS_WITH_LIMITATION" } else { "FAIL" }
$Summary = @(
	"Breachborne packaged reconnect attempt: $Result",
	"Timestamp: $(Get-Date -Format o)",
	"Initial gameplay: PASS",
	"Transport reconnect: $(if ($JoinCount -eq 3) { 'PASS' } else { 'FAIL' })",
	"Match-state restoration: UNSUPPORTED (replacement joins as spectator)",
	"Successful joins: $JoinCount",
	"Evidence: $RunDirectory"
)
if ($Failures.Count -gt 0) {
	$Summary += "Failures:"
	$Summary += $Failures | ForEach-Object { "- $_" }
}
$Summary | Set-Content -LiteralPath (Join-Path $RunDirectory "ReconnectSummary.txt") -Encoding UTF8
$Summary | ForEach-Object { Write-Host $_ }

if ($Failures.Count -gt 0) {
	exit 1
}
