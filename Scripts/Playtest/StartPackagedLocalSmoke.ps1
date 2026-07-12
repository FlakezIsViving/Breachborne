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
	[switch]$NoSound = $true
)

$ErrorActionPreference = "Stop"

if ($ClientCount -lt 1 -or $ClientCount -gt 4) {
	throw "ClientCount must be between 1 and 4."
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
		RunDirectory = $RunDirectory
		Address = $Address
		Port = $Port
		ClientCount = $ClientCount
		Processes = $Processes
	}
	$SessionPath = Join-Path $RunDirectory "Session.json"
	$Session | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $SessionPath -Encoding UTF8
	$RunDirectory | Set-Content -LiteralPath (Join-Path $OutputRoot "LatestSession.txt") -Encoding UTF8

	Write-Host "Breachborne interactive packaged session started."
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
