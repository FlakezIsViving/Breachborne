param(
	[string]$ProjectPath = "$PSScriptRoot\..\..\Breachborne.uproject",
	[string]$EngineRoot = "C:\UnrealEngine-5.7.4-release",
	[switch]$Log,
	[switch]$Trace,
	[switch]$Wait,
	[switch]$DryRun,
	[string[]]$AdditionalArguments = @()
)

$ErrorActionPreference = "Stop"

$Editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf)) {
	throw "UnrealEditor.exe not found: $Editor"
}
if (-not (Test-Path -LiteralPath $ProjectPath -PathType Leaf)) {
	throw "Project not found: $ProjectPath"
}

$ResolvedProject = (Resolve-Path -LiteralPath $ProjectPath).ProviderPath
$ProjectDescriptor = Get-Content -LiteralPath $ResolvedProject -Raw | ConvertFrom-Json
$DisabledPlugins = @(
	$ProjectDescriptor.Plugins |
		Where-Object { $_.Enabled -eq $false } |
		ForEach-Object { [string]$_.Name } |
		Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
)

$Arguments = @("`"$ResolvedProject`"")
if ($DisabledPlugins.Count -gt 0) {
	$Arguments += "-DisablePlugins=$($DisabledPlugins -join ',')"
}
if ($Log) {
	$Arguments += "-log"
}
if ($Trace) {
	$Arguments += @("-trace=cpu,frame,bookmark,loadtime,file", "-statnamedevents")
}
$Arguments += $AdditionalArguments
$ArgumentLine = $Arguments -join " "

Write-Host "Editor: $Editor"
Write-Host "Arguments: $ArgumentLine"
if ($DryRun) {
	return
}

$ExistingEditor = Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'" |
	Where-Object { $_.CommandLine -like "*$ResolvedProject*" } |
	Select-Object -First 1
if ($ExistingEditor) {
	Write-Host "Breachborne editor is already running with PID $($ExistingEditor.ProcessId)."
	return
}

$Process = Start-Process -FilePath $Editor -ArgumentList $ArgumentLine -WorkingDirectory $EngineRoot -PassThru
Write-Host "Started UnrealEditor.exe with PID $($Process.Id)."

if ($Wait) {
	Wait-Process -Id $Process.Id
}
