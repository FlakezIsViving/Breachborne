param(
	[string]$ProjectPath = "$PSScriptRoot\..\..\Breachborne.uproject",
	[string]$Map = "/Game/Maps/TestMap",
	[int]$Port = 7777,
	[string]$UnrealEditorExe = "C:\UnrealEngine-5.7.4-release\Engine\Binaries\Win64\UnrealEditor.exe"
)

$ResolvedProject = Resolve-Path -LiteralPath $ProjectPath

if (-not (Test-Path -LiteralPath $UnrealEditorExe)) {
	throw "UnrealEditor.exe not found at '$UnrealEditorExe'. Pass -UnrealEditorExe or update the script default."
}

& $UnrealEditorExe $ResolvedProject $Map -server -log -port=$Port -nosteam -NoSound
