param(
	[string]$ClientBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsClient",
	[string]$ServerBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsServer",
	[string]$Address = "127.0.0.1:7777",
	[int]$ClientCount = 2,
	[int]$Port = 7777,
	[int]$ServerStartupSeconds = 5,
	[int]$ResX = 900,
	[int]$ResY = 506,
	[string]$OutputRoot = "$PSScriptRoot\..\..\Saved\Logs\InteractivePlaytest",
	[string]$SessionLabel = "",
	[string[]]$ServerExtraArgs = @(),
	[string[]]$ClientExtraArgs = @(),
	[switch]$VerifyCandidate,
	[switch]$ValidateOnly,
	[switch]$NoSound = $true
)

$ErrorActionPreference = "Stop"

if ($ClientCount -lt 1 -or $ClientCount -gt 4) {
	throw "ClientCount must be between 1 and 4."
}

$LatestSessionPath = Join-Path $OutputRoot "LatestSession.txt"
if (Test-Path -LiteralPath $LatestSessionPath) {
	$PreviousDirectory = (Get-Content -LiteralPath $LatestSessionPath -Raw).Trim()
	$PreviousSessionPath = Join-Path $PreviousDirectory "Session.json"
	if (Test-Path -LiteralPath $PreviousSessionPath) {
		$PreviousSession = Get-Content -LiteralPath $PreviousSessionPath -Raw | ConvertFrom-Json
		$ActiveRoles = @()
		foreach ($Entry in $PreviousSession.Processes) {
			$Process = Get-Process -Id ([int]$Entry.PID) -ErrorAction SilentlyContinue
			if ($Process -and $Process.Path -and [string]::Equals(
				[IO.Path]::GetFullPath($Process.Path),
				[IO.Path]::GetFullPath([string]$Entry.Executable),
				[System.StringComparison]::OrdinalIgnoreCase)) {
				$ActiveRoles += [string]$Entry.Role
			}
		}
		if ($ActiveRoles.Count -gt 0) {
			throw "Recorded session is still active ($($ActiveRoles -join ', ')): $PreviousDirectory. Run StopPackagedLocalSmoke.ps1 first."
		}
	}
}

if ($VerifyCandidate) {
	$global:LASTEXITCODE = 0
	& (Join-Path $PSScriptRoot "VerifyPlaytestCandidate.ps1")
	if ($LASTEXITCODE -ne 0) {
		throw "Playtest candidate verification failed. Visible clients were not launched."
	}
}

if (Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue) {
	throw "UDP port $Port is already in use. Pass a different -Port value."
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
	throw "Packaged client/server executables were not found. Package both targets first."
}

if ($ValidateOnly) {
	Write-Host "Breachborne interactive packaged launcher validation: PASS"
	Write-Host "  Clients: $ClientCount"
	Write-Host "  Address: $Address"
	Write-Host "  Port:    UDP $Port (available)"
	Write-Host "  Client:  $($ClientExe.FullName)"
	Write-Host "  Server:  $($ServerExe.FullName)"
	Write-Host "No processes were launched."
	$global:LASTEXITCODE = 0
	return
}

$Timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$RunDirectory = Join-Path $OutputRoot $Timestamp
New-Item -ItemType Directory -Force -Path $RunDirectory | Out-Null
$Processes = @()

try {
	$ServerLog = Join-Path $RunDirectory "Server.log"
	$ServerArgs = @(
		"/Game/Maps/TestMap",
		"-port=$Port",
		"-nosteam",
		"-log",
		"-abslog=`"$ServerLog`""
	)
	if ($NoSound) {
		$ServerArgs += "-NoSound"
	}
	$ServerArgs += $ServerExtraArgs

	$Server = Start-Process -FilePath $ServerExe.FullName -ArgumentList $ServerArgs `
		-WindowStyle Hidden -PassThru
	$Processes += [pscustomobject]@{
		Role = "Server"
		PID = $Server.Id
		Executable = $ServerExe.FullName
		Log = $ServerLog
	}

	Start-Sleep -Seconds $ServerStartupSeconds
	if ($Server.HasExited) {
		throw "Packaged server exited during startup with code $($Server.ExitCode). See $ServerLog"
	}

	for ($Index = 1; $Index -le $ClientCount; $Index++) {
		$ClientLog = Join-Path $RunDirectory "Client$Index.log"
		$WindowColumn = ($Index - 1) % 2
		$WindowRow = [math]::Floor(($Index - 1) / 2)
		$WinX = 10 + ($WindowColumn * ($ResX + 20))
		$WinY = 30 + ($WindowRow * ($ResY + 40))
		$ClientArgs = @(
			$Address,
			"-game",
			"-windowed",
			"-ResX=$ResX",
			"-ResY=$ResY",
			"-WinX=$WinX",
			"-WinY=$WinY",
			"-nosteam",
			"-log",
			"-abslog=`"$ClientLog`""
		)
		if ($NoSound) {
			$ClientArgs += "-NoSound"
		}
		$ClientArgs += $ClientExtraArgs

		$Client = Start-Process -FilePath $ClientExe.FullName -ArgumentList $ClientArgs `
			-WindowStyle Normal -PassThru
		$Processes += [pscustomobject]@{
			Role = "Client$Index"
			PID = $Client.Id
			Executable = $ClientExe.FullName
			Log = $ClientLog
		}
	}

	$Session = [ordered]@{
		StartedAt = (Get-Date -Format o)
		SessionLabel = $SessionLabel
		RunDirectory = $RunDirectory
		Address = $Address
		Port = $Port
		ClientCount = $ClientCount
		ServerExtraArgs = $ServerExtraArgs
		ClientExtraArgs = $ClientExtraArgs
		Processes = $Processes
	}
	$SessionPath = Join-Path $RunDirectory "Session.json"
	$Session | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $SessionPath -Encoding UTF8
	$RunDirectory | Set-Content -LiteralPath $LatestSessionPath -Encoding UTF8

	Write-Host "Breachborne interactive packaged session started."
	if (-not [string]::IsNullOrWhiteSpace($SessionLabel)) {
		Write-Host "  Session:  $SessionLabel"
	}
	Write-Host "  Clients:  $ClientCount"
	Write-Host "  Address:  $Address"
	Write-Host "  Evidence: $RunDirectory"
	Write-Host "  Stop:     .\Scripts\Playtest\StopPackagedLocalSmoke.ps1"
	Write-Host "Follow docs\plans\july-31-manual-vfx-acceptance.md and test owner plus observer views."
}
catch {
	foreach ($Started in $Processes) {
		$Process = Get-Process -Id $Started.PID -ErrorAction SilentlyContinue
		if ($Process) {
			Stop-Process -Id $Process.Id -Force -ErrorAction SilentlyContinue
		}
	}
	throw
}
