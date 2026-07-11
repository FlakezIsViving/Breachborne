param(
	[string]$ClientBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsClient",
	[string]$ServerBuildRoot = "$PSScriptRoot\..\..\Builds\WindowsServer",
	[string]$Address = "127.0.0.1:7777",
	[int]$ClientCount = 2,
	[int]$Port = 7777
)

$ErrorActionPreference = "Stop"

$ServerScript = Join-Path $PSScriptRoot "StartPackagedServer.ps1"
$ClientScript = Join-Path $PSScriptRoot "StartPackagedClient.ps1"

Start-Process -FilePath "powershell.exe" -ArgumentList @(
	"-NoExit",
	"-ExecutionPolicy", "Bypass",
	"-File", "`"$ServerScript`"",
	"-BuildRoot", "`"$ServerBuildRoot`"",
	"-Port", $Port
)

Start-Sleep -Seconds 5

for ($i = 0; $i -lt $ClientCount; $i++) {
	$X = 80 + ($i * 40)
	$Y = 80 + ($i * 40)
	& $ClientScript `
		-BuildRoot $ClientBuildRoot `
		-Address $Address `
		-WinX $X `
		-WinY $Y
}

Write-Host "Started packaged local smoke test with $ClientCount clients."
