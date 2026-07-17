param(
	[string]$ProjectPath = "C:\Unreal Projects\Breachborne\Breachborne.uproject",
	[string]$EngineRoot = "C:\UnrealEngine-5.7.4-release",
	[switch]$Wait,
	[switch]$DryRun
)

& "$PSScriptRoot\..\Editor\StartSourceEditor.ps1" `
	-ProjectPath $ProjectPath `
	-EngineRoot $EngineRoot `
	-Log `
	-Trace `
	-Wait:$Wait `
	-DryRun:$DryRun
